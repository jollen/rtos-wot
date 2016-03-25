# rtos-wot

[![Join the chat at https://gitter.im/jollen/rtos-wot](https://badges.gitter.im/jollen/rtos-wot.svg)](https://gitter.im/jollen/rtos-wot?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

[rtos-wot](https://github.com/wot-sdk/rtos-wot) 是一個用於學習 FreeRTOS + IoT 的 FreeRTOS 特別版本（distribution），目前使用在我自已的 FreeRTOS + IoT 教育訓練。

為什麼需要一個特別的 FreeRTOS 版本？主要原因是官方的 ESP8266 RTOS SDK 沒有開放源碼，因此計畫維護一份單純用於教育用途的 FreeRTOS 套件。說明如下：

* 使用 Open Source 的 [FreeRTOS](http://www.freertos.org) 原始碼，能研究並修改 FreeRTOS kernel 原始碼
* 使用 Open Source 的 lwip 做為 TCP/IP Stacks，能研究 FreeRTOS kernel 原始碼
* 使用 Contiki 的 CoAP (er-coap) 程式庫，不但有原始碼，也支援較高階的 CoAP APIs，能進行 CoAP 訊息與格式的教學
* 硬體控制部份，需要一個 Component-based 的程式設計模式
* 具開放源碼的 FreeRTOS + lwip + CoAP 能進行完整的 Web of Things 全端教學

詳細的技術細節與發佈紀錄，請參考 [README_rtos-wot.md](README_rtos-wot.md)。

## 其它

* rtos-wot 較適合用於學習用途，正式 IoT 開發，請使用官方的 [ESP8266 RTOS SDK](https://github.com/espressif/ESP8266_RTOS_SDK)
* rtos-wot 基於 Superhouse 的 esp-open-rtos，請參考 [esp-open-rtos](https://github.com/jollen/rtos-wot/blob/master/README_esp-open-rtos.md)

## 課程紀錄

* 2016.3.24: [FreeRTOS & lwIP 物聯網應用開發](https://www.moko365.com/enterprise/iot100-freertos-iot-programming-101)

## 安裝

rtos-wot 環境建置指南，請參考 [使用 ESP8266 做為 FreeRTOS 的學習與開發環境](http://www.jollen.org/blog/2016/01/study-freertos-using-esp8266.html)


## Licensing

* BSD license (as described in LICENSE) applies to original source files, [lwIP](http://lwip.wikia.com/wiki/LwIP_Wiki). lwIP is Copyright (C) Swedish Institute of Computer Science.

* FreeRTOS is provided under the GPL with the FreeRTOS linking exception, allowing non-GPL firmwares to be produced using FreeRTOS as the RTOS core. License details in files under FreeRTOS dir. FreeRTOS is Copyright (C) Real Time Engineers Ltd.

* Source & binary components from the [Espressif IOT RTOS SDK](https://github.com/espressif/esp_iot_rtos_sdk) were released under the MIT license. Source code components are relicensed here under the BSD license. The original parts are Copyright (C) Espressif Systems.

* Newlib is covered by several copyrights and licenses, as per the files in the `libc` directory.

* [mbedTLS](https://tls.mbed.org/) is provided under the Apache 2.0 license as described in the file extras/mbedtls/mbedtls/apache-2.0.txt. mbedTLS is Copyright (C) ARM Limited.

* Components under `extras/` may contain different licenses, please see those directories for details.

* CoAP as in the `coap` directory is Copyright (c) 2013, Institute for Pervasive Computing, ETH Zurich

* The examples for [FreeRTOS & lwIP 物聯網應用開發](https://www.moko365.com/enterprise/iot100-freertos-iot-programming-101), except the examples included in the original FreeRTOS distribution, are provided under the BSD 3-Clause license
