/**
 * @file
 * ARM mbed ADC component
 *
 */

/*
 * * Copyright (c) 2016 WoT.City Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is a demo of RTOS WoT open source project. Please visit the project
 * web site https://github.com/wot-sdk/rtos-wot for documentation.
 *
 * Author: Jollen Chen <jollen@jollen.org>
 *
 * This is a ESP8266 ADC demo. It aims to reuse the ARM mbed component.
 *
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
void ICACHE_FLASH_ATTR
readTask(void *pvParameters)
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

void ICACHE_FLASH_ATTR
transmitTask(void *pvParameters)
{
    struct userdata *user = (struct userdata *)pvParameters;

    float temperature;
    int a, b;

    while(1) {
        // Suspend ourselves.
        vTaskSuspend( NULL );

        // Mutex & Semaphore ?
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
