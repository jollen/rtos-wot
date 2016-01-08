/* The classic "blink" example
 *
 * This sample code is in the public domain.
 */
#include "espressif/esp_common.h"
#include "esp/uart.h"
#include "FreeRTOS.h"
#include "task.h"
#include "esp8266.h"

// mbed API layer
#include "DigitalOut.h"
#include "AnalogIn.h"

// GPIO2 (D4) 
DigitalOut D4(2);

/* This task uses the high level GPIO API (esp_gpio.h) to blink an LED.
 *
 */
void blinkenTask(void *pvParameters)
{
    while(1) {
        D4 = 1;
        wait(1)
        D4 = 0;
        wait(3);
    }
}

extern "C" void user_init(void)
{
    uart_set_baud(0, 115200);
    xTaskCreate(blinkenTask, (signed char *)"blinkenTask", 256, NULL, 2, NULL);
}
