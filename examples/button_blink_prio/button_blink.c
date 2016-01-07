/* Respond to a button press.
 *
 * This code combines two ways of checking for a button press -
 * busy polling (the bad way) and button interrupt (the good way).
 *
 * This sample code is in the public domain.
 */
#include "espressif/esp_common.h"
#include "esp/uart.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "esp8266.h"

struct userdata {
    xQueueHandle xQueue;
    xTaskHandle xHandle;
};

/* user context */
static struct userdata user;

/* pin config */
const int gpio_blink = 2;
const int gpio = 14;   /* gpio 0 usually has "PROGRAM" button attached */
const int active = 0; /* active == 0 for active low */
const gpio_inttype_t int_type = GPIO_INTTYPE_EDGE_NEG;
#define GPIO_HANDLER gpio14_interrupt_handler

/* This task configures the GPIO interrupt and uses it to tell
   when the button is pressed.

   The interrupt handler communicates the exact button press time to
   the task via a queue.

   This is a better example of how to wait for button input!
*/
void buttonIntTask(void *pvParameters)
{
    printf("Waiting for button press interrupt on gpio %d...\r\n", gpio);
    struct userdata *user = (struct userdata *)pvParameters;
    gpio_set_interrupt(gpio, int_type);

    uint32_t last = 0;
    while(1) {
        uint32_t button_ts;
        xQueueReceive(user->xQueue, &button_ts, portMAX_DELAY);
        button_ts *= portTICK_RATE_MS;
        if(last < button_ts-200) {
            printf("Button interrupt fired at %dms\r\n", button_ts);
            last = button_ts;

            // Resume the suspended task ourselves.
            if( user->xHandle != NULL ) {
                vTaskResume( user->xHandle );
            }
        }
    }
}

/* This task uses the high level GPIO API (esp_gpio.h) to blink an LED.
 *
 */
void blinkenTask(void *pvParameters)
{
    while(1) {
        // Suspend ourselves.
        vTaskSuspend( NULL );

        gpio_write(gpio_blink, 1);
        vTaskDelay(1000 / portTICK_RATE_MS);
        gpio_write(gpio_blink, 0);
    }
}

void GPIO_HANDLER(void)
{
    uint32_t now = xTaskGetTickCountFromISR();
    xQueueSendToBackFromISR(user.xQueue, &now, NULL);
}

void user_init(void)
{
    uart_set_baud(0, 115200);
    gpio_enable(gpio, GPIO_INPUT);
    gpio_enable(gpio_blink, GPIO_OUTPUT);

    user.xQueue = xQueueCreate(2, sizeof(uint32_t));
    xTaskCreate(buttonIntTask, (signed char *)"buttonIntTask", 256, &user, tskIDLE_PRIORITY+1, NULL);
    xTaskCreate(blinkenTask, (signed char*)"blinkenTask", 256, NULL, tskIDLE_PRIORITY, &user.xHandle);
}
