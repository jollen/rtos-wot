/* http_get - Retrieves a web page over HTTP GET.
 *
 * See http_get_ssl for a TLS-enabled version.
 *
 * This sample code is in the public domain.,
 */
#include "espressif/esp_common.h"
#include "esp/uart.h"

#include <string.h>

#include "FreeRTOS.h"
#include "task.h"

#include "math.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#include "ssid_config.h"

#define WEB_SERVER "wot.city"
#define WEB_PORT "80"

const static int B = 4250;

void http_get_task(void *pvParameters)
{
    char cmd[120];
    int successes = 0, failures = 0;
    int ret = 0;

    int a;
    //int b;
    int vin;
    float temperature;
    float R;

    unsigned char frame;
    char *token;

    printf("HTTP get task starting...\r\n");

    while(1) {
        // 1. Socket type
        const struct addrinfo hints = {
            .ai_family = AF_INET,
            .ai_socktype = SOCK_STREAM,
        };
        struct addrinfo *res;

        // 2. DNS lookup
        printf("Running DNS lookup for %s...\r\n", WEB_SERVER);
        int err = getaddrinfo(WEB_SERVER, WEB_PORT, &hints, &res);

        if(err != 0 || res == NULL) {
            printf("DNS lookup failed err=%d res=%p\r\n", err, res);
            if(res)
                freeaddrinfo(res);
            vTaskDelay(1000 / portTICK_RATE_MS);
            failures++;
            continue;
        }
        /* Note: inet_ntoa is non-reentrant, look at ipaddr_ntoa_r for "real" code */
        struct in_addr *addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
        printf("DNS lookup succeeded. IP=%s\r\n", inet_ntoa(*addr));

        // 3. Create a local socket
        int s = socket(res->ai_family, res->ai_socktype, 0);
        if(s < 0) {
            printf("... Failed to allocate socket.\r\n");
            freeaddrinfo(res);
            vTaskDelay(1000 / portTICK_RATE_MS);
            failures++;
            continue;
        }

        printf("... allocated socket\r\n");

        // 4. Connect to server
        if(connect(s, res->ai_addr, res->ai_addrlen) != 0) {
            close(s);
            freeaddrinfo(res);
            printf("... socket connect failed.\r\n");
            vTaskDelay(4000 / portTICK_RATE_MS);
            failures++;
            continue;
        }

        printf("... connected\r\n");
        freeaddrinfo(res);

        //send websocket HTTP header
        sprintf(cmd, "GET /object/5550937980d51931b3000009/send HTTP/1.1\r\n");
        ret = write(s, cmd, strlen(cmd));
        if (ret < 0) goto again;

        sprintf(cmd, "Upgrade: websocket\r\n");
        ret = write(s, cmd, strlen(cmd));
        if (ret < 0) goto again;

        sprintf(cmd, "Connection: Upgrade\r\n");
        ret = write(s, cmd, strlen(cmd));
        if (ret < 0) goto again;

        sprintf(cmd, "Sec-WebSocket-Version: 13\r\n");
        ret = write(s, cmd, strlen(cmd));
        if (ret < 0) goto again;

        sprintf(cmd, "Sec-WebSocket-Key: L159VM0TWUzyDxwJEIEzjw==\r\n");
        ret = write(s, cmd, strlen(cmd));
        if (ret < 0) goto again;

        sprintf(cmd, "Host: %s\r\n", "wot.city");
        ret = write(s, cmd, strlen(cmd));
        if (ret < 0) goto again;

        sprintf(cmd, "Origin: null\r\n\r\n");
        ret = write(s, cmd, strlen(cmd));
        if (ret < 0) goto again;

        // handshake from the server
        // Sec-WebSocket-Accept: DdLWT/1JcX+nQFHebYP+rqEx5xI=
        static char recv_buf[512];
        do {
            bzero(recv_buf, 512);
            ret = read(s, recv_buf, 511);
            if(ret > 0) {
                printf("[LEN: %d] %s", ret, recv_buf);
            }            
            token = strtok(recv_buf, "\r\n\r\n");
            if (token != NULL) 
                break;
        } while(ret > 0);

        printf("Websocket handshake success.\r\nLast read return=%d errno=%d\r\n", ret, errno);
        if(ret != 0)
            failures++;
        else
            successes++;

        printf("successes = %d failures = %d\r\n", successes, failures);

        // Success.
        printf("\r\nWebsocket connection created.\r\n");

        do {
            bzero(cmd, 125);
            vTaskDelay(1000 / portTICK_RATE_MS);

            // read from sensor output voltage
            vin = sdk_system_adc_read();
            R = (float)(1023-vin)*10000/vin;       
            temperature = 1 / (log(R/10000)/B+1/298.15) - 273.15;

            // precision
            temperature = temperature * 100;
            a = (int)temperature / 100;
            //b = (int)temperature % 100;
            sprintf(cmd, "{\"temperature\":%d}\r\n", a);

            // websocket frame
            frame = 0x01; // FIN
            frame = (0x01 << 4); // WebSocketOpcode.TEXT_FRAME
            ret = write(s, &frame, 1);
            printf("[RET: %d] 0x%04x, ", ret, frame);

            frame = strlen(cmd);
            frame = (frame << 1);  // Payload length
            frame = (frame & 0x01); // Mask (Must be 1)
            ret = write(s, &frame, 1);
            printf("[RET: %d] 0x%04x, ", ret, frame);
            
            ret = write(s, cmd, strlen(cmd)); // Payload
            printf("[RET: %d] %s", ret, cmd);
        } while(ret >= 0);

again:
        printf("... socket send failed\r\n");
        close(s);
        vTaskDelay(5000 / portTICK_RATE_MS);
        failures++;
        continue;
    }
}

void user_init(void)
{
    uart_set_baud(0, 115200);
    printf("SDK version:%s\n", sdk_system_get_sdk_version());

    struct sdk_station_config config = {
        .ssid = WIFI_SSID,
        .password = WIFI_PASS,
    };

    /* required to call wifi_set_opmode before station_set_config */
    sdk_wifi_set_opmode(STATION_MODE);
    sdk_wifi_station_set_config(&config);

    xTaskCreate(&http_get_task, (signed char *)"get_task", 256, NULL, 2, NULL);
}

