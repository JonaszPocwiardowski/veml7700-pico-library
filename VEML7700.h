#ifndef VEML7700_H
#define VEML7700_H

#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include <stdbool.h>

#define VEML7700_I2C_ADDRESS 0x10
#define VEML7700_REG_ALS_CONF 0x00
#define VEML7700_REG_ALS_WH 0x01
#define VEML7700_REG_ALS_WL 0x02
#define VEML7700_REG_POWER_SAVING 0x03
#define VEML7700_REG_ALS 0x04
#define VEML7700_REG_WHITE 0x05
#define VEML7700_REG_ALS_INT 0x06
#define VEML7700_REG_ID 0x07

typedef enum
{
    VEML7700_GAIN_1 = 0b00,
    VEML7700_GAIN_2 = 0b01,
    VEML7700_GAIN_1_8 = 0b10,
    VEML7700_GAIN_1_4 = 0b11
} veml7700_gain_t;

typedef enum
{
    VEML7700_IT_25MS = 0b1100,
    VEML7700_IT_50MS = 0b1000,
    VEML7700_IT_100MS = 0b0000,
    VEML7700_IT_200MS = 0b0001,
    VEML7700_IT_400MS = 0b0010,
    VEML7700_IT_800MS = 0b0011
} veml7700_it_t;

typedef enum
{
    VEML7700_PERS_1 = 0b00,
    VEML7700_PERS_2 = 0b01,
    VEML7700_PERS_4 = 0b10,
    VEML7700_PERS_8 = 0b11
} veml7700_pers_t;

typedef enum
{
    VEML7700_PSM_1 = 0b00,
    VEML7700_PSM_2 = 0b01,
    VEML7700_PSM_3 = 0b10,
    VEML7700_PSM_4 = 0b11
} veml7700_psm_t;

#define VEML7700_INT_FLAG_TH_LOW (1 << 14)
#define VEML7700_INT_FLAG_TH_HIGH (1 << 15)

typedef struct
{
    i2c_inst_t *i2c_port;
    uint16_t config_cache;
    uint16_t psm_cache;
    bool initialized;
    veml7700_gain_t current_gain;
    veml7700_it_t current_it;
} veml7700_sensor_t;

#define VEML7700_OK 0
#define VEML7700_ERROR_INIT -1
#define VEML7700_ERROR_I2C_TX -2
#define VEML7700_ERROR_I2C_RX -3
#define VEML7700_ERROR_CONFIG -4
#define VEML7700_ERROR_INVALID_ARG -5

/**
 * @brief Initializes the VEML7700 sensor.
 *
 * Configures the I2C instance (if not already configured), checks the device ID,
 * sets the default configuration (Gain x1, IT 100ms, Pers 1, INT disabled, PSM off),
 * and enables the sensor.
 *
 * @param sensor Pointer to the structure holding the sensor state.
 * @param i2c_port Pointer to the I2C instance (e.g., i2c0 or i2c1).
 * @param sda_pin SDA pin number.
 * @param scl_pin SCL pin number.
 * @param baudrate I2C bus speed (e.g., 100000 or 400000).
 * @return VEML7700_OK on success, error code otherwise.
 */
int veml7700_init(veml7700_sensor_t *sensor, i2c_inst_t *i2c_port, uint sda_pin, uint scl_pin, uint baudrate);

/**
 * @brief Reads the raw value from the ALS (Ambient Light Sensor) converter (16-bit).
 *
 * @param sensor Pointer to the initialized sensor structure.
 * @param als_value Pointer to the variable where the read value will be stored.
 * @return VEML7700_OK on success, error code otherwise.
 */
int veml7700_read_als(veml7700_sensor_t *sensor, uint16_t *als_value);

/**
 * @brief Reads the raw value from the white channel (16-bit).
 *
 * @param sensor Pointer to the initialized sensor structure.
 * @param white_value Pointer to the variable where the read value will be stored.
 * @return VEML7700_OK on success, error code otherwise.
 */
int veml7700_read_white(veml7700_sensor_t *sensor, uint16_t *white_value);

/**
 * @brief Calculates and reads the illuminance value in lux.
 *
 * Automatically takes into account the current gain and integration time settings.
 * Reads the configuration register before calculation to ensure the current settings are used.
 *
 * @param sensor Pointer to the initialized sensor structure.
 * @param lux_value Pointer to a float variable where the calculated lux value will be stored.
 * @return VEML7700_OK on success, error code otherwise.
 */
int veml7700_read_lux(veml7700_sensor_t *sensor, float *lux_value);

/**
 * @brief Sets the sensor gain.
 *
 * @param sensor Pointer to the initialized sensor structure.
 * @param gain New gain setting.
 * @return VEML7700_OK on success, error code otherwise.
 */
int veml7700_set_gain(veml7700_sensor_t *sensor, veml7700_gain_t gain);

/**
 * @brief Sets the sensor integration time.
 *
 * @param sensor Pointer to the initialized sensor structure.
 * @param it New integration time setting.
 * @return VEML7700_OK on success, error code otherwise.
 */
int veml7700_set_integration_time(veml7700_sensor_t *sensor, veml7700_it_t it);

/**
 * @brief Sets the number of persistence measurements for interrupts.
 *
 * Specifies how many consecutive measurements must exceed the threshold to generate an interrupt.
 *
 * @param sensor Pointer to the initialized sensor structure.
 * @param pers New persistence setting.
 * @return VEML7700_OK on success, error code otherwise.
 */
int veml7700_set_persistence(veml7700_sensor_t *sensor, veml7700_pers_t pers);

/**
 * @brief Enables or disables the interrupt mechanism.
 *
 * Sets the ALS_INT_EN bit in the configuration register.
 * Note that the VEML7700 does not have a physical interrupt pin. The interrupt status
 * must be read from the ALS_INT register (0x06) using the
 * veml7700_read_interrupt_status() function.
 *
 * @param sensor Pointer to the initialized sensor structure.
 * @param enable True to enable interrupts, false to disable.
 * @return VEML7700_OK on success, error code otherwise.
 */
int veml7700_enable_interrupt(veml7700_sensor_t *sensor, bool enable);

/**
 * @brief Sets the upper threshold (High Threshold) for the ALS interrupt.
 *
 * The threshold value is directly written to the ALS_WH register.
 * This is a 16-bit raw ADC value.
 *
 * @param sensor Pointer to the initialized sensor structure.
 * @param threshold 16-bit threshold value.
 * @return VEML7700_OK on success, error code otherwise.
 */
int veml7700_set_high_threshold(veml7700_sensor_t *sensor, uint16_t threshold);

/**
 * @brief Sets the lower threshold (Low Threshold) for the ALS interrupt.
 *
 * The threshold value is directly written to the ALS_WL register.
 * This is a 16-bit raw ADC value.
 *
 * @param sensor Pointer to the initialized sensor structure.
 * @param threshold 16-bit threshold value.
 * @return VEML7700_OK on success, error code otherwise.
 */
int veml7700_set_low_threshold(veml7700_sensor_t *sensor, uint16_t threshold);

/**
 * @brief Reads the interrupt status from the ALS_INT register (0x06).
 *
 * Returns the 16-bit value of the register. Bits 14 (ALS_IF_L) and 15 (ALS_IF_H)
 * indicate whether the lower or upper threshold has been crossed, respectively.
 * These flags are automatically cleared after reading the ALS_INT register.
 * You can use the VEML7700_INT_FLAG_TH_LOW and VEML7700_INT_FLAG_TH_HIGH masks
 * to check specific flags.
 *
 * @param sensor Pointer to the initialized sensor structure.
 * @param interrupt_status Pointer to the variable where the 16-bit
 * value of the interrupt status register will be stored.
 * @return VEML7700_OK on success, error code otherwise.
 */
int veml7700_read_interrupt_status(veml7700_sensor_t *sensor, uint16_t *interrupt_status);

/**
 * @brief Enables or disables the power saving mode (PSM).
 *
 * @param sensor Pointer to the initialized sensor structure.
 * @param enable True to enable PSM, false to disable.
 * @return VEML7700_OK on success, error code otherwise.
 */
int veml7700_enable_power_saving(veml7700_sensor_t *sensor, bool enable);

/**
 * @brief Sets the power saving mode (PSM).
 *
 * Only works when PSM is enabled (see veml7700_enable_power_saving).
 *
 * @param sensor Pointer to the initialized sensor structure.
 * @param psm New PSM mode.
 * @return VEML7700_OK on success, error code otherwise.
 */
int veml7700_set_power_saving_mode(veml7700_sensor_t *sensor, veml7700_psm_t psm);

/**
 * @brief Enables the sensor (exits shutdown mode).
 *
 * @param sensor Pointer to the initialized sensor structure.
 * @return VEML7700_OK on success, error code otherwise.
 */
int veml7700_power_on(veml7700_sensor_t *sensor);

/**
 * @brief Disables the sensor (enters shutdown mode - low power consumption).
 *
 * Current consumption typically 0.5 uA [source: 112 from veml7700.pdf].
 *
 * @param sensor Pointer to the initialized sensor structure.
 * @return VEML7700_OK on success, error code otherwise.
 */
int veml7700_shutdown(veml7700_sensor_t *sensor);

/**
 * @brief Reads the device ID.
 *
 * @param sensor Pointer to the initialized sensor structure.
 * @param device_id Pointer to the variable where the ID (16-bit) will be stored.
 * @return VEML7700_OK on success, error code otherwise.
 */
int veml7700_read_device_id(veml7700_sensor_t *sensor, uint16_t *device_id);

#endif