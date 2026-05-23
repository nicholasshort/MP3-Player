#include "bq24259.h"
#include "i2c.h"
#include <stdint.h>
#include <stddef.h>

#define BQ24259_DEVICE_ADDRESS      ((0x6B) << 1)
#define BQ24259_I2C_TIMEOUT_MS      100

#define BQ24259_VENDER_PART_REVISION_STATUS_REGISTER                0x0A
#define BQ24259_CHARGE_TERMINATION_TIMER_CONTROL_REGISTER           0x05
#define BQ24259_POWER_ON_CONFIGURATION_REGISTER                     0x01
#define BQ24259_INPUT_SOURCE_CONTROL_REGISTER                       0x00
#define BQ24259_CHARGE_CURRENT_CONTROL_REGISTER                     0x02
#define BQ24259_PRE_CHARGE_TERMINATION_CURRENT_CONTROL_REGISTER     0x03
#define BQ24259_CHARGE_VOLTAGE_CONTROL_REGISTER                     0x04

static bq24259_result_e bq24259_reg_read(uint8_t reg, uint8_t* value) {

    if (value == NULL)
        return BQ24259_RESULT_INVALID_ARG;
    
    if (HAL_I2C_Mem_Read(&hi2c1, BQ24259_DEVICE_ADDRESS, reg, I2C_MEMADD_SIZE_8BIT, value, 1, BQ24259_I2C_TIMEOUT_MS) != HAL_OK)
        return BQ24259_RESULT_ERROR_I2C;

    return BQ24259_RESULT_OK;

}

static bq24259_result_e bq24259_reg_write(uint8_t reg, uint8_t value) {
    
    if (HAL_I2C_Mem_Write(&hi2c1, BQ24259_DEVICE_ADDRESS, reg, I2C_MEMADD_SIZE_8BIT, &value, 1, BQ24259_I2C_TIMEOUT_MS) != HAL_OK)
        return BQ24259_RESULT_ERROR_I2C;

    return BQ24259_RESULT_OK;

}

static bq24259_result_e bq24259_reg_write_verify(uint8_t reg, uint8_t value) {

    bq24259_result_e ret;
    ret = bq24259_reg_write(reg, value);
    if (ret != BQ24259_RESULT_OK)
        return ret;
    
    uint8_t new_reg;
    ret = bq24259_reg_read(reg, &new_reg);
    if (ret != BQ24259_RESULT_OK)
        return ret;

    if (new_reg != value)
        return BQ24259_RESULT_ERROR_VERIFY;

    return BQ24259_RESULT_OK;

}

static bq24259_result_e bq24259_reg_update_bits(uint8_t reg, uint8_t mask, uint8_t value) {

    uint8_t old_value;
    bq24259_result_e ret;
    ret = bq24259_reg_read(reg, &old_value);
    if (ret != BQ24259_RESULT_OK)
        return ret;

    uint8_t new_value = ( old_value & (uint8_t)~mask ) | ( value & mask);
    ret = bq24259_reg_write(reg, new_value);
    if (ret != BQ24259_RESULT_OK)
        return ret;

    return BQ24259_RESULT_OK;

}

static bq24259_result_e bq24259_reg_update_bits_verify(uint8_t reg, uint8_t mask, uint8_t value) {

    bq24259_result_e ret;
    ret = bq24259_reg_update_bits(reg, mask, value);
    if (ret != BQ24259_RESULT_OK)
        return ret;

    uint8_t new_value;
    ret = bq24259_reg_read(reg, &new_value);
    if (ret != BQ24259_RESULT_OK)
        return ret;
    if ( (new_value & mask) != (value & mask) )
        return BQ24259_RESULT_ERROR_VERIFY;

    return BQ24259_RESULT_OK;

}

bq24259_result_e bq24259_init(void) {

    if (HAL_I2C_IsDeviceReady(&hi2c1, BQ24259_DEVICE_ADDRESS, 3, BQ24259_I2C_TIMEOUT_MS) != HAL_OK)
        return BQ24259_RESULT_ERROR_I2C;
    
    // Ensure we have correct part revision
    uint8_t reg0A;
    bq24259_result_e ret = bq24259_reg_read(BQ24259_VENDER_PART_REVISION_STATUS_REGISTER, &reg0A);
    if (ret != BQ24259_RESULT_OK)
        return ret;
    if (reg0A != 0x20)
        return BQ24259_RESULT_BAD_ID;
    
    // Disable watchdog for now (Set Bit 5 and 4 to 0) and ensure fast charge termination timer set for 12 hrs
    ret = bq24259_reg_write_verify(BQ24259_CHARGE_TERMINATION_TIMER_CONTROL_REGISTER, (0x8C));
    if (ret != BQ24259_RESULT_OK)
        return ret;
    
    // Set VSYS_MIN to 3.7V (Only valid when USB and Battery present) (Set 3:1 to 111)
    ret = bq24259_reg_update_bits_verify(BQ24259_POWER_ON_CONFIGURATION_REGISTER, (0x1E), (0x1E));
    if (ret != BQ24259_RESULT_OK)
        return ret;
    
    // Set IINLIM to 500mA (at least at beginning) (Set 2:0 to 010)
    ret = bq24259_reg_update_bits_verify(BQ24259_INPUT_SOURCE_CONTROL_REGISTER, (0x07), (0x02));
    if (ret != BQ24259_RESULT_OK)
        return ret;
    
    // Set ICHG to 384mA (at least at beginning) (Set 6:2 to 00110)
    ret = bq24259_reg_update_bits_verify(BQ24259_CHARGE_CURRENT_CONTROL_REGISTER, (0x1F << 2), (0x18));
    if (ret != BQ24259_RESULT_OK)
        return ret;
    
    // Set precharge current (IPRECHG) to 128mA for longevity (Set 7:4 to 0001)
    // Set termination current (ITERM) to 128mA (lowest setting) for fuller battery (Set 2:0 to 000)
    ret = bq24259_reg_write_verify(BQ24259_PRE_CHARGE_TERMINATION_CURRENT_CONTROL_REGISTER, (0x10));
    if (ret != BQ24259_RESULT_OK)
        return ret;

    // Set voltage charge control to default configuration: 4.208 max charge voltage, 3.0V precharge -> fast charge threshold, 100mV Recharge Threshold
    ret = bq24259_reg_write_verify(BQ24259_CHARGE_VOLTAGE_CONTROL_REGISTER, (0xB2));
    if (ret != BQ24259_RESULT_OK)
        return ret;

    return BQ24259_RESULT_OK;

}
