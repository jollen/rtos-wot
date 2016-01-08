/* The classic "blink" example
 *
 * This sample code is in the public domain.
 */
#include "espressif/esp_common.h"
#include "esp/uart.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "esp8266.h"
#include "math.h"

// C++ programming model
#include "AnalogIn.h"

// CoAP
#include "er-coap-13.h"
#include "er-coap-13-transactions.h"
#include "uri.h"

#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define COAP_PRINTF(...) printf(__VA_ARGS__)
#else
#define COAP_PRINTF(...)
#endif

#define MAX_MESSAGE_SIZE 1152

/* CoAP server */
const char* SERVER_URI = "coap://wot.city/";

struct userdata {
    xQueueHandle xQueue;
    xTaskHandle xHandle;

    /* CoAP context */
    coap_transaction_t *transaction;
    coap_packet_t *response;
};

/* user context */
static struct userdata user;

/* ADC0 (A0)
 *
 * MP503 Air Quality Sensor -
 * http://www.seeedstudio.com/wiki/File:Air_quality_sensor_MP503.pdf
 */
AnalogIn    AIR(17);

/* This task uses the high level GPIO API (esp_gpio.h) to blink an LED.
 *
 */
void
readTask(void *pvParameters)
{
    struct userdata *user = (struct userdata *)pvParameters;
    int a;

    while(1) {
        // read from sensor output voltage
        a = AIR;

        if (a > 798 || a <= 10) {
            printf("Sensor is initializing. Waiting for 5 seconds...\n");
            wait(5);
            continue;
        }

        // send to queue
        xQueueSendToBack( user->xQueue, (void *) &a, portMAX_DELAY );

        wait(5);
    }
}

void
transmitTask(void *pvParameters)
{
    struct userdata *user = (struct userdata *)pvParameters;
    coap_packet_t request[1]; /* This way the packet can be treated as pointer as usual. */

    int a;
    char payload[128];
    int failures = 0;

    // Get host/port from request URL
    // should call free(uri) somewhere
    coap_uri_t *uri = coap_new_uri(SERVER_URI, strlen(SERVER_URI));   
    if (uri == NULL) {
        COAP_PRINTF("coap_new_uri(): URI failed\n");
        vTaskDelete(NULL);
    }

    COAP_PRINTF("URI Path: %s\n", uri->path.s);
    COAP_PRINTF("URI Host: %s\n", uri->host.s);

    // init a HTTP POST message and set message header
    coap_init_message(request, COAP_TYPE_CON, COAP_POST, 0);
    coap_set_header_uri_path(request, (char *)uri->path.s);
    coap_set_header_uri_host(request, (char *)uri->host.s);

    // create network socket
    struct addrinfo hints;
    struct addrinfo *res;

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    // 2. DNS lookup
    int err;
    char port[6];

    sprintf(port, "%d", uri->port);
    printf("Running DNS lookup for %s...\r\n", uri->host.s);

    while (1) {
        err = getaddrinfo((char*)uri->host.s, (char*)port, &hints, &res);
        if (err == 0)
            break;
        freeaddrinfo(res);
        printf("DNS lookup failed err=%d res=%p\r\n", err, res);
        vTaskDelay(3000 / portTICK_RATE_MS);
    }

    /* Note: inet_ntoa is non-reentrant, look at ipaddr_ntoa_r for "real" code */
    struct in_addr *addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
    printf("DNS lookup succeeded. IP=%s\r\n", inet_ntoa(*addr));

    // 3. Create a local socket
    int s = socket(res->ai_family, res->ai_socktype, 0);
    if(s < 0) {
        printf("... Failed to allocate socket.\r\n");
        freeaddrinfo(res);
        vTaskDelete(NULL);
    }

    printf("... allocated socket\r\n");

    int ret;

    while(1) {
        // connect to server
        if (connect(s, res->ai_addr, res->ai_addrlen) != 0) {
            close(s);
            freeaddrinfo(res);
            failures++;
            printf("... socket connect failed. tries = %d.\r\n", failures);
            vTaskDelay(4000 / portTICK_RATE_MS);
            continue;
        }

        printf("... connected\r\n");
        freeaddrinfo(res);

repeat:
        xQueueReceive(user->xQueue, &a, portMAX_DELAY);

        sprintf(payload, "{ \"quality\": %d }\n", a);
        COAP_PRINTF("Payload: %s\n", payload);

        // CoAP payload
        coap_set_payload(request, payload, strlen(payload));

        ret = write(s, payload, strlen(payload));
        if (ret >= 0) goto repeat;
    }
}

extern "C" void user_init(void)
{
    uart_set_baud(0, 115200);

    user.xQueue = xQueueCreate(2, sizeof(uint32_t));
    xTaskCreate(readTask, (signed char *)"readTask", 256, &user, tskIDLE_PRIORITY+1, NULL);
    xTaskCreate(transmitTask, (signed char*)"transmitTask", 256, &user, tskIDLE_PRIORITY, &user.xHandle);
}
