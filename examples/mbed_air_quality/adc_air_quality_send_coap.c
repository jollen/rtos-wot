/**
 * @file
 * CoAP Push Client
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
 * This is a CoAP client for the lwIP TCP/IP stack. It aims to conform
 * with RFC 7252.
 *
 */
 
#include "espressif/esp_common.h"
#include "esp/uart.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "esp8266.h"
#include "math.h"

// C++ programming model
//#include "AnalogIn.h"

// CoAP
#include "er-coap-13.h"
#include "er-coap-13-transactions.h"
#include "uri.h"

#include "ssid_config.h"

#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define COAP_PRINTF(...) printf(__VA_ARGS__)
#else
#define COAP_PRINTF(...)
#endif

#define MAX_MESSAGE_SIZE 1152

#define SERVER_URI  "coap://wot.city:8147/object/12345/send"

#define wait(s) vTaskDelay(s * 1000 / portTICK_RATE_MS);

struct userdata {
    xQueueHandle xQueue;
    xTaskHandle xHandle;

    /* CoAP context */
    coap_transaction_t *transaction;
    coap_packet_t *response;

    /* CoAP server */
    unsigned char server_uri[64];
};

/* user context */
static struct userdata user;

/* ADC0 (A0)
 *
 * MP503 Air Quality Sensor -
 * http://www.seeedstudio.com/wiki/File:Air_quality_sensor_MP503.pdf
 */
//AnalogIn    AIR(17);

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
        a = 99;//AIR;
        //printf("Quality: %d\n", a);

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

    // Prepare CoAP server URI
    memset(user->server_uri, 0, 64);
    memcpy(user->server_uri, SERVER_URI, strlen(SERVER_URI));

    // Get host/port from request URL
    // should call free(uri) somewhere
    coap_uri_t *uri = coap_new_uri(user->server_uri, strlen((const char*)user->server_uri));   
    if (uri == NULL) {
        COAP_PRINTF("coap_new_uri(): URI failed\n");
        vTaskDelete(NULL);
    }

    // create network socket
    struct addrinfo hints;
    struct addrinfo *res;

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    // 2. DNS lookup
    int err;
    char port[6];
    char host[32];
    char path[64];

    sprintf(port, "%d", uri->port);
    sprintf(path, "%s", uri->path.s);
    memcpy(host, uri->host.s, uri->host.length);
    host[uri->host.length] = '\0';

    COAP_PRINTF("URI Path: %s\n", path);
    COAP_PRINTF("URI Host: %s\n", host);
    COAP_PRINTF("URI Port: %s\n", port);

    printf("Running DNS lookup for %s...\r\n", host);

    // init a HTTP POST message and set message header
    coap_init_message(request, COAP_TYPE_CON, COAP_POST, 0);
    coap_set_header_uri_path(request, (char *)uri->path.s);
    coap_set_header_uri_host(request, (char *)uri->host.s);

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

    user.xQueue = xQueueCreate(2, sizeof(uint32_t));
    xTaskCreate(readTask, (signed char *)"readTask", 256, &user, tskIDLE_PRIORITY+1, NULL);
    xTaskCreate(transmitTask, (signed char*)"transmitTask", 256, &user, tskIDLE_PRIORITY, &user.xHandle);
}
