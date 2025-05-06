[![License](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
![Platform](https://img.shields.io/badge/Platform-Raspberry_Pi_Pico-orange.svg)
[![Language](https://img.shields.io/badge/Language-C-blue.svg)](https://en.wikipedia.org/wiki/C_(programming_language))
![I2C](https://img.shields.io/badge/Interface-I2C-lightgrey.svg)
[![Build](https://img.shields.io/badge/Build-CMake%20%2B%20Make-success.svg)](https://cmake.org/)


# VEML7700 Library for Raspberry Pi Pico

A comprehensive C library for operating the **VEML7700** ambient light sensor (ALS) from Vishay Semiconductors on the **Raspberry Pi Pico (RP2040/RP2350)** microcontroller using the I2C interface.

The library is designed with ease of use in mind, offering functions for sensor initialization, data reading in various formats (raw ADC values, calculated lux), and advanced configuration options such as gain, integration time, interrupt thresholds, and power-saving modes.

## Key Features

* Sensor and I2C communication initialization.
* Reading raw 16-bit values from the ALS channel.
* Reading raw 16-bit values from the White channel (if needed).
* Reading calculated ambient light intensity in **lux (lx)**.
* Gain configuration (x1/8, x1/4, x1, x2).
* Integration time configuration (25ms - 800ms).
* Configuration of interrupt persistence settings.
* Interrupt handling (enable/disable, status reading, setting lower and upper thresholds).  
  *Note: The VEML7700 does not have a physical INT pin; interrupt status is read via software.*
* Power-saving modes (PSM) support.
* Sensor enable/disable (shutdown mode).
* Device ID reading.
* Error handling via return codes.
* Optional debug messages (`VEML7700_DEBUG`).

## Hardware Requirements

* Raspberry Pi Pico or a compatible board with RP2040 or RP2350.
* VEML7700 sensor module.
* I2C connection between the Pico and the VEML7700 module (VCC, GND, SDA, SCL).  
  Remember to use pull-up resistors on SDA and SCL lines if not already present on the sensor module.

## Software Requirements

* **Pico SDK:** A configured development environment for the Raspberry Pi Pico.
* **CMake:** Build system used by the Pico SDK.

## Installation / Project Integration

1. **Copy the files:** Place the `veml7700.c` and `veml7700.h` files into your project directory (e.g., in a subdirectory like `lib/veml7700/` or directly in the main project folder).
2. **Update `CMakeLists.txt`:** Add the source files and include path in your project's `CMakeLists.txt`:

    ```cmake
    cmake_minimum_required(VERSION 3.13)

    # Initialize Pico SDK (adjust the path if needed)
    include($ENV{PICO_SDK_PATH}/pico_sdk_init.cmake)

    project(my_veml7700_project C CXX ASM)
    set(CMAKE_C_STANDARD 11)
    set(CMAKE_CXX_STANDARD 17)

    # Initialize the SDK
    pico_sdk_init()

    # Name of your executable
    set(TARGET_NAME my_veml7700_project)
    add_executable(${TARGET_NAME}
        main.c # Your main source file
        # Add the VEML7700 library source file
        veml7700.c # or path like lib/veml7700/veml7700.c
    )

    # Add the path to the VEML7700 header file
    target_include_directories(${TARGET_NAME} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR} # or path like lib/veml7700
    )

    # Link required Pico SDK libraries
    target_link_libraries(${TARGET_NAME} PRIVATE
        pico_stdlib
        hardware_i2c
    )

    # Optional: enable stdio via USB/UART
    pico_enable_stdio_usb(${TARGET_NAME} 1)
    pico_enable_stdio_uart(${TARGET_NAME} 0)

    # Generate .uf2, .hex, etc.
    pico_add_extra_outputs(${TARGET_NAME})
    ```

3. **Build the project:** Use the standard CMake and Make (or other generator) build process.

## Usage Examples

### Basic Ambient Light Reading (Lux)

```c
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "veml7700.h" // Include the library header

// I2C pin configuration
#define I2C_PORT i2c0
#define I2C_SDA_PIN 4
#define I2C_SCL_PIN 5
#define I2C_BAUDRATE 100000 // 100 kHz

int main() {
    // Initialize standard I/O (for printf)
    stdio_init_all();
    sleep_ms(2000); // Delay to allow terminal to open
    printf("Start - VEML7700 example\n");

    // Initialize I2C
    i2c_init(I2C_PORT, I2C_BAUDRATE);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);

    // Sensor state structure
    veml7700_sensor_t sensor;

    // Initialize the VEML7700 sensor
    int status = veml7700_init(&sensor, I2C_PORT, I2C_SDA_PIN, I2C_SCL_PIN, I2C_BAUDRATE);

    if (status != VEML7700_OK) {
        printf("VEML7700 initialization error: %d\n", status);
        while (1); // Halt on error
    } else {
        printf("VEML7700 successfully initialized.\n");
    }

    // Read device ID (optional)
    uint16_t device_id;
    status = veml7700_read_device_id(&sensor, &device_id);
    if (status == VEML7700_OK) {
        printf("Device ID: 0x%04X\n", device_id);
    }

    // Power on the sensor (it's enabled by default after init, but can be toggled)
    veml7700_power_on(&sensor);
    sleep_ms(10); // Short pause after power on

    while (1) {
        float lux_value;
        uint16_t als_raw, white_raw;

        // Read ambient light in lux
        status = veml7700_read_lux(&sensor, &lux_value);
        if (status == VEML7700_OK) {
            printf("Ambient light: %.4f lx\n", lux_value);
        } else {
            printf("Lux read error: %d\n", status);
        }

        // Read raw ALS value (optional)
        status = veml7700_read_als(&sensor, &als_raw);
        if (status == VEML7700_OK) {
            printf("Raw ALS: %u\n", als_raw);
        } else {
            printf("ALS read error: %d\n", status);
        }

        // Read raw White channel value (optional)
        status = veml7700_read_white(&sensor, &white_raw);
        if (status == VEML7700_OK) {
            printf("Raw White: %u\n", white_raw);
        } else {
            printf("White read error: %d\n", status);
        }

        printf("----\n");
        sleep_ms(1000); // Wait 1 second before next reading
    }

    return 0;
}
```
## Changing Configuration (Gain and Integration Time)
```c
#include "veml7700.h"
// ... (rest of the code as above, including initialization)

int main() {
    // ... I2C and veml7700_init initialization ...
    if (status != VEML7700_OK) { /* error handling */ }

    printf("Configuring VEML7700...\n");

    // Set gain to x1/4
    status = veml7700_set_gain(&sensor, VEML7700_GAIN_1_4);
    if (status != VEML7700_OK) {
        printf("Failed to set gain: %d\n", status);
    } else {
        printf("Gain set to x1/4.\n");
    }
    sleep_ms(5); // Short pause after configuration change

    // Set integration time to 400ms
    status = veml7700_set_integration_time(&sensor, VEML7700_IT_400MS);
    if (status != VEML7700_OK) {
        printf("Failed to set integration time: %d\n", status);
    } else {
        printf("Integration time set to 400ms.\n");
    }
    sleep_ms(5);

    // ... reading loop as in the previous example ...
    // Lux readings will now reflect the new settings
}
```
## API - Function Overview
The library functions operate on a pointer to the `veml7700_sensor_t` structure, which stores the sensor state and I2C configuration. Below is a description of the main functions available in `veml7700.h`.

## Initialization and Basics
* `int veml7700_init(veml7700_sensor_t *sensor, i2c_inst_t *i2c_port, uint sda_pin, uint scl_pin, uint baudrate)`: Initializes the sensor structure, sets up the I2C port (if not already configured), verifies the device ID, applies default settings, and powers on the sensor. Returns `VEML7700_OK` or an error code.

* `int veml7700_read_device_id(veml7700_sensor_t *sensor, uint16_t *device_id)`: Reads the 16-bit device ID.

## Data Reading

* `int veml7700_read_als(veml7700_sensor_t *sensor, uint16_t *als_value)`: Reads the raw 16-bit value from the main ALS channel.
* `int veml7700_read_white(veml7700_sensor_t *sensor, uint16_t *white_value)`: Reads the raw 16-bit value from the white channel.
* `int veml7700_read_lux(veml7700_sensor_t *sensor, float *lux_value)`: Reads the ALS value and converts it to lux, taking into account the current gain and integration time settings. Reads the configuration register before calculation to ensure up-to-date settings.

## Sensor Configuration
* `int veml7700_set_gain(veml7700_sensor_t *sensor, veml7700_gain_t gain)`: Sets the sensor gain. Available values: `VEML7700_GAIN_1_8`, `VEML7700_GAIN_1_4`, `VEML7700_GAIN_1`, `VEML7700_GAIN_2`.
* `int veml7700_set_integration_time(veml7700_sensor_t *sensor, veml7700_it_t it)`: Sets the integration time for measurements. Available values: `VEML7700_IT_25MS`, `VEML7700_IT_50MS`, `VEML7700_IT_100MS`, `VEML7700_IT_200MS`, `VEML7700_IT_400MS`, `VEML7700_IT_800MS`.
* `int veml7700_set_persistence(veml7700_sensor_t *sensor, veml7700_pers_t pers)`: Sets the number of consecutive threshold-exceeding readings required to trigger the interrupt flag. Values: `VEML7700_PERS_1`, `VEML7700_PERS_2`, `VEML7700_PERS_4`, `VEML7700_PERS_8`.

## Interrupt Handling (Software)
* `int veml7700_enable_interrupt(veml7700_sensor_t *sensor, bool enable)`: Enables (`true`) or disables (`false`) the generation of interrupt flags in the status register.
* `int veml7700_set_high_threshold(veml7700_sensor_t *sensor, uint16_t threshold)`: Sets the upper interrupt threshold (16-bit raw value).
* `int veml7700_set_low_threshold(veml7700_sensor_t *sensor, uint16_t threshold)`: Sets the lower interrupt threshold (16-bit raw value).
* `int veml7700_read_interrupt_status(veml7700_sensor_t *sensor, uint16_t *interrupt_status)`: Reads the 16-bit interrupt status register. Use the masks `VEML7700_INT_FLAG_TH_LOW` (bit 14) and `VEML7700_INT_FLAG_TH_HIGH` (bit 15) to check which flag was triggered. Reading this register clears the flags.

## Power Management
* `int veml7700_shutdown(veml7700_sensor_t *sensor)`: Puts the sensor into low-power shutdown mode.
* `int veml7700_power_on(veml7700_sensor_t *sensor)`: Wakes the sensor from shutdown mode. A short delay (~3ms) is required before reading data.
* `int veml7700_enable_power_saving(veml7700_sensor_t *sensor, bool enable)`: Enables (`true`) or disables (`false`) the power-saving mode (PSM).
* `int veml7700_set_power_saving_mode(veml7700_sensor_t *sensor, veml7700_psm_t psm)`: Sets the specific PSM mode (measurement frequency in power-saving mode) when PSM is enabled. Values: `VEML7700_PSM_1`, `VEML7700_PSM_2`, `VEML7700_PSM_3`, `VEML7700_PSM_4`.

## Configuration Options (Enums)
### Gain (`veml7700_gain_t`)
Controls the sensor sensitivity. Higher gain allows measurement of lower light levels but saturates faster under bright light.
* `VEML7700_GAIN_1_8`: Gain x1/8 (0.125)
* `VEML7700_GAIN_1_4`: Gain x1/4 (0.25)
* `VEML7700_GAIN_1`: Gain x1 (default)
* `VEML7700_GAIN_2`: Gain x2

### Integration Time (`veml7700_it_t`)
Specifies how long the sensor integrates light for a single measurement. Longer times improve resolution and sensitivity in low-light conditions but slow down sampling and increase saturation risk under bright light.
* `VEML7700_IT_25MS`: 25 ms
* `VEML7700_IT_50MS`: 50 ms
* `VEML7700_IT_100MS`: 100 ms (default)
* `VEML7700_IT_200MS`: 200 ms
* `VEML7700_IT_400MS`: 400 ms
* `VEML7700_IT_800MS`: 800 ms

### Persistence (`veml7700_pers_t`)
Sets how many consecutive measurements must exceed threshold limits (high or low) before the corresponding interrupt flag is set. Useful for filtering out short-term fluctuations in light.
* `VEML7700_PERS_1`: 1 sample (default)
* `VEML7700_PERS_2`: 2 samples
* `VEML7700_PERS_4`: 4 samples
* `VEML7700_PERS_8`: 8 samples

### Power Saving Mode (`veml7700_psm_t`)
When PSM is enabled (`veml7700_enable_power_saving`), this setting defines how often the sensor takes a measurement, helping reduce energy consumption between samples.
* `VEML7700_PSM_1`: Measurement period = (Integration Time + 500 ms)
* `VEML7700_PSM_2`: Measurement period = (Integration Time + 1000 ms)
* `VEML7700_PSM_3`: Measurement period = (Integration Time + 2000 ms)
* `VEML7700_PSM_4`: Measurement period = (Integration Time + 4000 ms)

## Error Handling
Functions return `VEML7700_OK` (equal to 0) on success or a negative error code otherwise.
* `VEML7700_OK` (0): Operation completed successfully.
* `VEML7700_ERROR_INIT` (-1): Initialization error (e.g., device not found or wrong ID).
* `VEML7700_ERROR_I2C_TX` (-2): I2C transmission error (write).
* `VEML7700_ERROR_I2C_RX` (-3): I2C transmission error (read).
* `VEML7700_ERROR_CONFIG` (-4): Configuration error (e.g., function called before initialization or `sensor` pointer is NULL).
* `VEML7700_ERROR_INVALID_ARG` (-5): Invalid function argument (e.g., result pointer is NULL).

Always check the return value of library functions!

## Debugging
To enable diagnostic messages (`printf` outputs), define the macro `VEML7700_DEBUG` before including the `veml7700.h` header, or pass the `-DVEML7700_DEBUG` flag at compile time.

Example (in your .c file):
```c
#define VEML7700_DEBUG
#include "veml7700.h"
```
Or in `CMakeLists.txt`:
```Cmake
target_compile_definitions(${TARGET_NAME} PRIVATE VEML7700_DEBUG)
```
