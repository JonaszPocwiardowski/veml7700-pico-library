#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "veml7700.h"

static int veml7700_write_reg(veml7700_sensor_t *sensor, uint8_t reg, uint16_t data)
{
    if (!sensor || !sensor->initialized)
    {
        return VEML7700_ERROR_CONFIG;
    }
    uint8_t buffer[3];
    buffer[0] = reg;
    buffer[1] = data & 0xFF;
    buffer[2] = (data >> 8) & 0xFF;

    int result = i2c_write_blocking(sensor->i2c_port, VEML7700_I2C_ADDRESS, buffer, 3, false);
    if (result < 0)
    {
#ifdef VEML7700_DEBUG
        printf("VEML7700 I2C Write Error: %d on Reg 0x%02X\n", result, reg);
#endif
        if (result == PICO_ERROR_GENERIC || result == PICO_ERROR_TIMEOUT)
        {
            return VEML7700_ERROR_I2C_TX;
        }
        return VEML7700_ERROR_I2C_TX;
    }
    if (result != 3)
    {
#ifdef VEML7700_DEBUG
        printf("VEML7700 I2C Write Error: Wrote %d bytes instead of 3 on Reg 0x%02X\n", result, reg);
#endif
        return VEML7700_ERROR_I2C_TX;
    }

    return VEML7700_OK;
}

static int veml7700_read_reg(veml7700_sensor_t *sensor, uint8_t reg, uint16_t *data)
{
    if (!sensor || !sensor->initialized)
    {
        return VEML7700_ERROR_CONFIG;
    }
    if (!data)
    {
        return VEML7700_ERROR_INVALID_ARG;
    }
    uint8_t buffer[2];

    int result = i2c_write_blocking(sensor->i2c_port, VEML7700_I2C_ADDRESS, &reg, 1, true);
    if (result < 0)
    {
#ifdef VEML7700_DEBUG
        printf("VEML7700 I2C Write Error (Read CMD): %d on Reg 0x%02X\n", result, reg);
#endif
        if (result == PICO_ERROR_GENERIC || result == PICO_ERROR_TIMEOUT)
        {
            return VEML7700_ERROR_I2C_TX;
        }
        return VEML7700_ERROR_I2C_TX;
    }
    if (result != 1)
    {
#ifdef VEML7700_DEBUG
        printf("VEML7700 I2C Write Error (Read CMD): Wrote %d bytes instead of 1 on Reg 0x%02X\n", result, reg);
#endif
        return VEML7700_ERROR_I2C_TX;
    }

    result = i2c_read_blocking(sensor->i2c_port, VEML7700_I2C_ADDRESS, buffer, 2, false);
    if (result < 0)
    {
#ifdef VEML7700_DEBUG
        printf("VEML7700 I2C Read Error: %d on Reg 0x%02X\n", result, reg);
#endif
        if (result == PICO_ERROR_GENERIC || result == PICO_ERROR_TIMEOUT)
        {
            return VEML7700_ERROR_I2C_RX;
        }
        return VEML7700_ERROR_I2C_RX;
    }
    if (result != 2)
    {
#ifdef VEML7700_DEBUG
        printf("VEML7700 I2C Read Error: Read %d bytes instead of 2 on Reg 0x%02X\n", result, reg);
#endif
        return VEML7700_ERROR_I2C_RX;
    }

    *data = (uint16_t)buffer[1] << 8 | buffer[0];
    return VEML7700_OK;
}

static float veml7700_get_resolution(veml7700_gain_t gain, veml7700_it_t it)
{
    float gain_val;
    float it_val;

    switch (gain)
    {
    case VEML7700_GAIN_1_8:
        gain_val = 0.125f;
        break;
    case VEML7700_GAIN_1_4:
        gain_val = 0.25f;
        break;
    case VEML7700_GAIN_1:
        gain_val = 1.0f;
        break;
    case VEML7700_GAIN_2:
        gain_val = 2.0f;
        break;
    default:
        gain_val = 1.0f;
        break;
    }

    switch (it)
    {
    case VEML7700_IT_25MS:
        it_val = 25.0f;
        break;
    case VEML7700_IT_50MS:
        it_val = 50.0f;
        break;
    case VEML7700_IT_100MS:
        it_val = 100.0f;
        break;
    case VEML7700_IT_200MS:
        it_val = 200.0f;
        break;
    case VEML7700_IT_400MS:
        it_val = 400.0f;
        break;
    case VEML7700_IT_800MS:
        it_val = 800.0f;
        break;
    default:
        it_val = 100.0f;
        break;
    }

    float base_resolution_at_ref = 0.0042f;
    float ref_gain_val = 2.0f;
    float ref_it_val = 800.0f;

    float resolution = base_resolution_at_ref * (ref_gain_val / gain_val) * (ref_it_val / it_val);

    if (resolution > 2.1504f)
    {
        resolution = 2.1504f;
    }
    if (resolution < 0.0042f)
    {
        resolution = 0.0042f;
    }

    return resolution;
}

int veml7700_init(veml7700_sensor_t *sensor, i2c_inst_t *i2c_port, uint sda_pin, uint scl_pin, uint baudrate)
{
    if (!sensor || !i2c_port)
    {
        return VEML7700_ERROR_INVALID_ARG;
    }

    sensor->i2c_port = i2c_port;
    sensor->initialized = false;

    sensor->initialized = true;
    int status;
    uint16_t device_id;

    status = veml7700_read_reg(sensor, VEML7700_REG_ID, &device_id);
    if (status != VEML7700_OK)
    {
        sensor->initialized = false;
#ifdef VEML7700_DEBUG
        printf("VEML7700 Init Error: Failed to read Device ID (Status: %d)\n", status);
#endif
        return VEML7700_ERROR_INIT;
    }

    if ((device_id & 0xFF) != 0x81 && (device_id >> 8) != 0x28)
    {
#ifdef VEML7700_DEBUG
        printf("VEML7700 Init Error: Wrong device ID: 0x%04X (Expected LSB 0x81 or MSB 0x28)\n", device_id);
#endif
        sensor->initialized = false;
        return VEML7700_ERROR_INIT;
    }
#ifdef VEML7700_DEBUG
    printf("VEML7700 Device ID: 0x%04X\n", device_id);
#endif

    sensor->current_gain = VEML7700_GAIN_1;
    sensor->current_it = VEML7700_IT_100MS;
    sensor->config_cache = (sensor->current_gain << 11) | (sensor->current_it << 6) | (VEML7700_PERS_1 << 4) | (0 << 1) | (0 << 0);
    status = veml7700_write_reg(sensor, VEML7700_REG_ALS_CONF, sensor->config_cache);
    if (status != VEML7700_OK)
    {
#ifdef VEML7700_DEBUG
        printf("VEML7700 Init Error: Failed to write ALS CONF (Status: %d)\n", status);
#endif
        sensor->initialized = false;
        return status;
    }

    sensor->psm_cache = 0x0000;
    status = veml7700_write_reg(sensor, VEML7700_REG_POWER_SAVING, sensor->psm_cache);
    if (status != VEML7700_OK)
    {
#ifdef VEML7700_DEBUG
        printf("VEML7700 Init Error: Failed to write PSM CONF (Status: %d)\n", status);
#endif
        sensor->initialized = false;
        return status;
    }

    status = veml7700_write_reg(sensor, VEML7700_REG_ALS_WH, 0xFFFF);
    if (status != VEML7700_OK)
    {
        sensor->initialized = false;
        return status;
    }
    status = veml7700_write_reg(sensor, VEML7700_REG_ALS_WL, 0x0000);
    if (status != VEML7700_OK)
    {
        sensor->initialized = false;
        return status;
    }

    sleep_ms(10);

    sensor->initialized = true;
#ifdef VEML7700_DEBUG
    printf("VEML7700 Initialized Successfully.\n");
#endif
    return VEML7700_OK;
}

int veml7700_read_als(veml7700_sensor_t *sensor, uint16_t *als_value)
{
    if (!sensor || !sensor->initialized)
        return VEML7700_ERROR_CONFIG;
    if (!als_value)
        return VEML7700_ERROR_INVALID_ARG;

    return veml7700_read_reg(sensor, VEML7700_REG_ALS, als_value);
}

int veml7700_read_white(veml7700_sensor_t *sensor, uint16_t *white_value)
{
    if (!sensor || !sensor->initialized)
        return VEML7700_ERROR_CONFIG;
    if (!white_value)
        return VEML7700_ERROR_INVALID_ARG;

    return veml7700_read_reg(sensor, VEML7700_REG_WHITE, white_value);
}

int veml7700_read_lux(veml7700_sensor_t *sensor, float *lux_value)
{
    if (!sensor || !sensor->initialized)
        return VEML7700_ERROR_CONFIG;
    if (!lux_value)
        return VEML7700_ERROR_INVALID_ARG;

    uint16_t als_raw;
    int status = veml7700_read_als(sensor, &als_raw);
    if (status != VEML7700_OK)
    {
        *lux_value = -1.0f;
        return status;
    }

    status = veml7700_read_reg(sensor, VEML7700_REG_ALS_CONF, &sensor->config_cache);
    if (status != VEML7700_OK)
    {
        *lux_value = -1.0f;
        return status;
    }
    sensor->current_gain = (veml7700_gain_t)((sensor->config_cache >> 11) & 0x03);
    sensor->current_it = (veml7700_it_t)((sensor->config_cache >> 6) & 0x0F);

    veml7700_gain_t gain = sensor->current_gain;
    veml7700_it_t it = sensor->current_it;

    float resolution = veml7700_get_resolution(gain, it);
    *lux_value = (float)als_raw * resolution;

    return VEML7700_OK;
}

int veml7700_set_gain(veml7700_sensor_t *sensor, veml7700_gain_t gain)
{
    if (!sensor || !sensor->initialized)
        return VEML7700_ERROR_CONFIG;

    sensor->config_cache &= ~((uint16_t)0b11 << 11);
    sensor->config_cache |= ((uint16_t)gain << 11);
    int status = veml7700_write_reg(sensor, VEML7700_REG_ALS_CONF, sensor->config_cache);
    if (status == VEML7700_OK)
    {
        sensor->current_gain = gain;
    }
    return status;
}

int veml7700_set_integration_time(veml7700_sensor_t *sensor, veml7700_it_t it)
{
    if (!sensor || !sensor->initialized)
        return VEML7700_ERROR_CONFIG;

    sensor->config_cache &= ~((uint16_t)0b1111 << 6);
    sensor->config_cache |= ((uint16_t)it << 6);
    int status = veml7700_write_reg(sensor, VEML7700_REG_ALS_CONF, sensor->config_cache);
    if (status == VEML7700_OK)
    {
        sensor->current_it = it;
    }
    return status;
}

int veml7700_set_persistence(veml7700_sensor_t *sensor, veml7700_pers_t pers)
{
    if (!sensor || !sensor->initialized)
        return VEML7700_ERROR_CONFIG;

    sensor->config_cache &= ~((uint16_t)0b11 << 4);
    sensor->config_cache |= ((uint16_t)pers << 4);
    return veml7700_write_reg(sensor, VEML7700_REG_ALS_CONF, sensor->config_cache);
}

int veml7700_enable_interrupt(veml7700_sensor_t *sensor, bool enable)
{
    if (!sensor || !sensor->initialized)
        return VEML7700_ERROR_CONFIG;

    if (enable)
    {
        sensor->config_cache |= (1 << 1);
    }
    else
    {
        sensor->config_cache &= ~(1 << 1);
    }
    return veml7700_write_reg(sensor, VEML7700_REG_ALS_CONF, sensor->config_cache);
}

int veml7700_set_high_threshold(veml7700_sensor_t *sensor, uint16_t threshold)
{
    if (!sensor || !sensor->initialized)
        return VEML7700_ERROR_CONFIG;

    return veml7700_write_reg(sensor, VEML7700_REG_ALS_WH, threshold);
}

int veml7700_set_low_threshold(veml7700_sensor_t *sensor, uint16_t threshold)
{
    if (!sensor || !sensor->initialized)
        return VEML7700_ERROR_CONFIG;

    return veml7700_write_reg(sensor, VEML7700_REG_ALS_WL, threshold);
}

int veml7700_read_interrupt_status(veml7700_sensor_t *sensor, uint16_t *interrupt_status)
{
    if (!sensor || !sensor->initialized)
        return VEML7700_ERROR_CONFIG;
    if (!interrupt_status)
        return VEML7700_ERROR_INVALID_ARG;

    return veml7700_read_reg(sensor, VEML7700_REG_ALS_INT, interrupt_status);
}

int veml7700_enable_power_saving(veml7700_sensor_t *sensor, bool enable)
{
    if (!sensor || !sensor->initialized)
        return VEML7700_ERROR_CONFIG;

    if (enable)
    {
        sensor->psm_cache |= (1 << 0);
    }
    else
    {
        sensor->psm_cache &= ~(1 << 0);
    }
    return veml7700_write_reg(sensor, VEML7700_REG_POWER_SAVING, sensor->psm_cache);
}

int veml7700_set_power_saving_mode(veml7700_sensor_t *sensor, veml7700_psm_t psm)
{
    if (!sensor || !sensor->initialized)
        return VEML7700_ERROR_CONFIG;

    sensor->psm_cache &= ~((uint16_t)0b11 << 1);
    sensor->psm_cache |= ((uint16_t)psm << 1);
    return veml7700_write_reg(sensor, VEML7700_REG_POWER_SAVING, sensor->psm_cache);
}

int veml7700_power_on(veml7700_sensor_t *sensor)
{
    if (!sensor || !sensor->initialized)
        return VEML7700_ERROR_CONFIG;

    sensor->config_cache &= ~(1 << 0);
    int status = veml7700_write_reg(sensor, VEML7700_REG_ALS_CONF, sensor->config_cache);
    if (status == VEML7700_OK)
    {
        sleep_ms(10);
    }
    return status;
}

int veml7700_shutdown(veml7700_sensor_t *sensor)
{
    if (!sensor || !sensor->initialized)
        return VEML7700_ERROR_CONFIG;

    sensor->config_cache |= (1 << 0);
    return veml7700_write_reg(sensor, VEML7700_REG_ALS_CONF, sensor->config_cache);
}

int veml7700_read_device_id(veml7700_sensor_t *sensor, uint16_t *device_id)
{
    if (!sensor || !sensor->initialized)
        return VEML7700_ERROR_CONFIG;
    if (!device_id)
        return VEML7700_ERROR_INVALID_ARG;

    return veml7700_read_reg(sensor, VEML7700_REG_ID, device_id);
}