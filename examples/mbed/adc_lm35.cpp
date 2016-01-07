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

// mbed API layer
#include "AnalogIn.h"

struct userdata {
    xQueueHandle xQueue;
    xTaskHandle xHandle;
};

/* user context */
static struct userdata user;

// ADC0 (A0)
// Temperature sensor at A0 (PIN-17)
AnalogIn    LM35(17);

/*
 * Constants
 */ 
// B-Constant. 
// See: http://www.seeedstudio.com/wiki/images/a/a1/NCP18WF104F03RC.pdf
const static int B = 4250;

/* This task uses the high level GPIO API (esp_gpio.h) to blink an LED.
 *
 */
void readTask(void *pvParameters)
{
    struct userdata *user = (struct userdata *)pvParameters;
    int a;
    float temperature;
    float R;

    while(1) {
        // read from sensor output voltage
        a = LM35;

        R = (float)(1023-a)*10000/a;       

        // convert to Celsius temperature
        temperature = 1 / (log(R/10000)/B+1/298.15) - 273.15;

        // precision
        temperature = temperature * 100;

        // send to queue
        xQueueSendToBack( user->xQueue, (void *) &temperature, portMAX_DELAY );

        // Resume the suspended task ourselves.
        if( user->xHandle != NULL ) {
            vTaskResume( user->xHandle );
        }

        wait(0.8);
    }
}

void transmitTask(void *pvParameters)
{
    struct userdata *user = (struct userdata *)pvParameters;

    float temperature;
    int a, b;

    while(1) {
        // Suspend ourselves.
        vTaskSuspend( NULL );

        xQueueReceive(user->xQueue, &temperature, portMAX_DELAY);

        a = (int)temperature / 100;
        b = (int)temperature % 100;

        printf("{ \"temperature\": %d.%d }\n", a, b);
    }
}

extern "C" void user_init(void)
{
    uart_set_baud(0, 115200);

    user.xQueue = xQueueCreate(2, sizeof(uint32_t));
    xTaskCreate(readTask, (signed char *)"readTask", 256, &user, tskIDLE_PRIORITY+1, NULL);
    xTaskCreate(transmitTask, (signed char*)"transmitTask", 256, &user, tskIDLE_PRIORITY, &user.xHandle);
}
