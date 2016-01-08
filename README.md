# rtos-wot

An open source FreeRTOS-based framework for ESP8266 microcontrollers web of things (WoT). Forked from [esp-open-rtos](https://github.com/SuperHouse/esp-open-rtos).

* Build reusable components for device drivers by GPIO/ADC/I2C/UART class library. 
* Ports of high-level WebSocket APIs.
* Ports of high-level CoAP APIs.
* etc.

The development is still in progress. 

## Reusable Components

Part of ARM mbed C++ class libraries is porting to rtos-wot project. This enables ESP8266 developers to write FreeRTOS appications in C++ Programming Model.

* [examples/mbed_lm35](https://github.com/wot-sdk/rtos-wot/tree/master/examples/mbed_lm35) - [mbed APIs](https://github.com/mbedmicro/mbed/tree/master/libraries/mbed/api) is a set of reusable hardware components. This example reuses *AnalogIn* to read the LM35 sensor data.
* [examples/mbed_air_quality](https://github.com/wot-sdk/rtos-wot/tree/master/examples/mbed_air_quality) - Reuse *AnalogIn* of mbed component to read the Air Quality sesnor.
* [examples/mbed_gpio](https://github.com/wot-sdk/rtos-wot/tree/master/examples/mbed_gpio) - Reuse *DigitalOut* of mbed component to control GPIOs.

To use the *AnalogIn* component, include its header file:

```
#include "AnalogIn.h"
```

To use the *AnalogIn* component and read sensor data from ESP8266 A0:

```
AnalogIn    AIR(17);
int a = AIR;
```

You can program ESP8266 FreeRTOS applications in mbed coding style.

## CoAP Library

*libcoap* of Contiki was fully ported to rtos-wot project. The following steps explains how to construct a CoAP message and transmit it to Internet with lwip.

### 1. Include necessary header files.

```
#include "er-coap-13.h"
#include "er-coap-13-transactions.h"
```

### 2. Prepare and initialize a CoAP packet.

```
coap_packet_t request[1];
coap_init_message(request, COAP_TYPE_CON, COAP_POST, 0);
```

In the example, the packet is initialized to *COAP_TYPE_CON* type and *POST* method.

### 3. Fill CoAP headers.

```
// 初始化 CoAP headers，填寫 CoAP header 的 Uri Path 與 Uri Host
coap_set_header_uri_path(request, 'object/123456/send');
coap_set_header_uri_host(request, 'wot.city');
```

In the example, the packet headers is filled with server URI path and host.

### 4. Set the payload.

```
const char *payload = "{}";
coap_set_payload(request, (uint8_t *)payload, strlen(payload));
```

The payload is the message context. In the example, the payload is an empty JSON object.

### 5. Get message ID

```
request->mid = coap_get_mid();
```

### 6. Serialize CoAP message

```
coap_transaction_t *transaction = coap_new_transaction(request->mid, &ipaddr, uri->port);
transaction->packet_len = coap_serialize_message(request, transaction->packet);
```

### 7. Final step

After serializing the CoAP message, the final CoAP packet is sotred at *transaction->packet*.

```
write(s, transaction->packet, transaction->packet_len);
```

CoAP is basis of web of things framework. For push pattern of WoT, the TD (thing description) can be serialized in CoAP binary format.

## History

* v0.1.0: 2015-01-06
 * Add open source component: *AnalogIn* and *DigitalOut* components from ARM mbed project

* v0.2.0: 2015-01-08
 * Add open source component: *er-coap-13* library from Contiki operating system project
 
## esp-open-rtos

Please read the original document [README_esp-open-rtos.md](README_esp-open-rtos.md) of [esp-open-rtos](https://github.com/SuperHouse/esp-open-rtos) project.
