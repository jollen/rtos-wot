/* mbed Microcontroller Library
 * Copyright (c) 2006-2013 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef MBED_ANALOGIN_H
#define MBED_ANALOGIN_H

#include "mbed.h"

/**
 * ESP8266_RTOS_SDK V1.3.0
 */
#include "espressif/esp_system.h"

/** An analog input, used for reading the voltage on a pin
 *
 * Example:
 * @code
 * // Print messages when the AnalogIn is greater than 50%
 *
 * #include "mbed.h"
 *
 * AnalogIn temperature(p20);
 *
 * int main() {
 *     while(1) {
 *         if(temperature > 0.5) {
 *             printf("Too hot! (%f)", temperature.read());
 *         }
 *     }
 * }
 * @endcode
 */
class AnalogIn {

public:

    /** Create an AnalogIn, connected to the specified pin
     *
     * @param pin AnalogIn pin to connect to
     * @param name (optional) A string to identify the object
     */
    AnalogIn(int pin) {
        adc = pin;
    }

    /** Read the input voltage, represented as a float in the range [0.0, 1.0]
     *
     * @returns A floating-point value representing the current input voltage, measured as a percentage
     */
    float read() {
        uint16_t tout = sdk_system_adc_read();
        // divide ain by 675 if ADC0 is set to 5V or 1023 if set to 3.3V
        float val = tout / 1023;
        return val; 
    }

    /** Read the input voltage, represented as an unsigned short in the range [0x0, 0xFFFF]
     *
     * @returns
     *   16-bit unsigned short representing the current input voltage, normalised to a 16-bit value
     */
    unsigned short read_u16() {
        if(adc == 17){
            return sdk_system_adc_read(); // readvdd33 is 12 bit
        }
        return 0xFFFF;
    }

#ifdef MBED_OPERATORS
    /** An operator shorthand for read()
     *
     * The float() operator can be used as a shorthand for read() to simplify common code sequences
     *
     * Example:
     * @code
     * float x = volume.read();
     * float x = volume;
     *
     * if(volume.read() > 0.25) { ... }
     * if(volume > 0.25) { ... }
     * @endcode
     */
    operator float() {
        return read();
    }

    /** An operator shorthand for read_u16()
     *
     * The int() operator can be used as a shorthand for read() to simplify common code sequences
     *
     * Example:
     * @code
     * int x = volume.read_u16();
     * int x = volume;
     *
     * if(volume.read() > 512) { ... }
     * if(volume > 100) { ... }
     * @endcode
     */
    operator int() {
        return read_u16();
    }
#endif

protected:
    int adc;
};


#endif