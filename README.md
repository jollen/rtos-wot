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

## esp-open-rtos

Please read the original document [README_esp-open-rtos.md](README_esp-open-rtos.md) of [esp-open-rtos](https://github.com/SuperHouse/esp-open-rtos) project.
