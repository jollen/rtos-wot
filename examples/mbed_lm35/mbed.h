#include "espressif/esp_common.h"
#include "esp/uart.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "esp8266.h"

#define MBED_OPERATORS 1

#define	wait(s)	vTaskDelay(s * 1000 / portTICK_RATE_MS);