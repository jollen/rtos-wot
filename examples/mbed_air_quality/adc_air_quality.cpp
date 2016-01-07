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

struct userdata {
    xQueueHandle xQueue;
    xTaskHandle xHandle;
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
void readTask(void *pvParameters)
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

        // Resume the suspended task ourselves.
        if( user->xHandle != NULL ) {
            vTaskResume( user->xHandle );
        }

        wait(5);
    }
}

void transmitTask(void *pvParameters)
{
    struct userdata *user = (struct userdata *)pvParameters;

    int a;

    while(1) {
        // Suspend ourselves.
        vTaskSuspend( NULL );

        xQueueReceive(user->xQueue, &a, portMAX_DELAY);

        printf("{ \"quality\": %d }\n", a);
    }
}

extern "C" void user_init(void)
{
    uart_set_baud(0, 115200);

    user.xQueue = xQueueCreate(2, sizeof(uint32_t));
    xTaskCreate(readTask, (signed char *)"readTask", 256, &user, tskIDLE_PRIORITY+1, NULL);
    xTaskCreate(transmitTask, (signed char*)"transmitTask", 256, &user, tskIDLE_PRIORITY, &user.xHandle);
}
