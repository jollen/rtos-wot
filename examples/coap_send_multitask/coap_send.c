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

#include <string.h>

#include "FreeRTOS.h"
#include "task.h"

// CoAP
#include "er-coap-13.h"
#include "er-coap-13-transactions.h"
#include "uri.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#include "ssid_config.h"

#define DEBUG 1
#if DEBUG
#define COAP_PRINTF(...) printf(__VA_ARGS__)
#else
#define COAP_PRINTF(...)
#endif

#define URI     "coap://192.168.0.102:8000/object/12345678/send"

struct request_state_t {
  coap_transaction_t *transaction;
  coap_packet_t *response;
  uint32_t block_num;
};

/*
 * CoAP request transaction callbock
 */
void coap_blocking_request_callback(void *callback_data, void *response) {
  struct request_state_t *state = (struct request_state_t *) callback_data;
  state->response = (coap_packet_t*) response;
  COAP_PRINTF("coap_blocking_request_callback is called.\n");
}

void
http_get_task(void *pvParameters)
{
    coap_packet_t request[1]; /* This way the packet can be treated as pointer as usual. */
    int failures = 0;

    // create network socket
    struct addrinfo hints;
    struct addrinfo *res;

    // Use an UDP socket
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    // Get host/port from request URL
    // should call free(uri) somewhere
    coap_uri_t *uri = coap_new_uri((unsigned char*)URI, strlen(URI));   
    if (uri == NULL) {
        COAP_PRINTF("coap_new_uri(): URI failed\n");
        vTaskDelete(NULL);
    }

    // DNS lookup
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

    while (1) {
        err = getaddrinfo(host, port, &hints, &res);
        if (err == 0)
            break;
        freeaddrinfo(res);
        printf("DNS lookup failed err=%d res=%p\r\n", err, res);
        vTaskDelay(3000 / portTICK_RATE_MS);
    }

    /* Note: inet_ntoa is non-reentrant, look at ipaddr_ntoa_r for "real" code */
    struct in_addr *addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
    char *ip = inet_ntoa(*addr);
    printf("DNS lookup succeeded. HOST=%s, IP=%s\r\n", host, ip);

    // init a HTTP POST message and set message header
    coap_init_message(request, COAP_TYPE_CON, COAP_POST, 0);
    coap_set_header_uri_path(request, path);
    coap_set_header_uri_host(request, host);

    // Create a local socket
    int s = socket(res->ai_family, res->ai_socktype, 0);
    if(s < 0) {
        printf("... Failed to allocate socket.\r\n");
        freeaddrinfo(res);
        vTaskDelete(NULL);
    }

    printf("... allocated socket\r\n");

    int a;
    int ret;
    char payload[128];
    struct request_state_t state[1];
    ip_addr_t ipaddr;

    ipaddr.addr = ipaddr_addr(ip);

    while(1) {
        if(connect(s, res->ai_addr, res->ai_addrlen) != 0) {
            close(s);
            freeaddrinfo(res);
            printf("... socket connect failed.\r\n");
            vTaskDelay(3000 / portTICK_RATE_MS);
            failures++;
            continue;
        }

        printf("... connected\r\n");
        freeaddrinfo(res);
repeat:
        vTaskDelay(5000 / portTICK_RATE_MS);

        // read sensor data from ESP8266 A0
        a = sdk_system_adc_read();

        sprintf(payload, "{ \"quality\": %d }\n", a);
        COAP_PRINTF("Payload: %s\n", payload);

        // CoAP payload
        coap_set_payload(request, payload, strlen(payload));

        // Make CoAP transaction
        request->mid = coap_get_mid();

        if ((state->transaction = coap_new_transaction(request->mid, &ipaddr, uri->port)))
        {
            state->transaction->callback = coap_blocking_request_callback;
            state->transaction->callback_data = state;

            if (state->block_num > 0)
            {
                coap_set_header_block2(request, state->block_num, 0, REST_MAX_CHUNK_SIZE);
            }

            // Build CoAP header and Options
            state->transaction->packet_len = coap_serialize_message(request, state->transaction->packet);

            COAP_PRINTF("Header dump: [0x%02X %02X %02X %02X]. Size: %d\n",
                request->buffer[0],
                request->buffer[1],
                request->buffer[2],
                request->buffer[3],
                state->transaction->packet_len
            );

            COAP_PRINTF("Requested #%u (MID %u)\n", state->block_num, request->mid);

            // Transmit
            ret = write(s, state->transaction->packet, state->transaction->packet_len);
            //ret = sendto(s, state->transaction->packet, state->transaction->packet_len);
            if (ret < 0) {
                printf("[RET: %d] CoAP packet send failed.\n", ret);
                continue;
            }
        }
        else
        {
          COAP_PRINTF("Could not allocate transaction buffer");
        }

        goto repeat;
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

    xTaskCreate(&http_get_task, (signed char *)"get_task", 256, NULL, tskIDLE_PRIORITY+1, NULL);
}

