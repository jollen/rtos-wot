/* The classic "blink" example
 *
 * This sample code is in the public domain.
 */
#include "espressif/esp_common.h"
#include "esp/uart.h"
#include "FreeRTOS.h"
#include "task.h"
#include "esp8266.h"

const int gpio = 2;

/* This task uses the high level GPIO API (esp_gpio.h) to blink an LED.
 *
 */
void ICACHE_FLASH_ATTR
blinkenTask(void *pvParameters)
{
    gpio_enable(gpio, GPIO_OUTPUT);
    while(1) {
        gpio_write(gpio, 1);
        vTaskDelay(1000 / portTICK_RATE_MS);
        gpio_write(gpio, 0);
        vTaskDelay(1000 / portTICK_RATE_MS);
    }
}

void user_init(void)
{
    uart_set_baud(0, 115200);
    xTaskCreate(blinkenTask, (signed char *)"blinkenTask", 256, NULL, 2, NULL);
}
