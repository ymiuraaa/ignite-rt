#ifndef _ICM20948_REGS_H_
#define _ICM20948_REGS_H_

#include <stdint.h>

typedef enum {
    // Common
    REG_BANK_SEL = 0x7F,

    // Bank 0
    ICM20948_REG_WHO_AM_I           = 0x00,
    ICM20948_REG_USER_CTRL          = 0x03,
    ICM20948_REG_LP_CONFIG          = 0x05,
    ICM20948_REG_PWR_MGMT_1         = 0x06,
    ICM20948_REG_PWR_MGMT_2         = 0x07,
    ICM20948_REG_INT_PIN_CONFIG     = 0x0F,
    ICM20948_REG_ACCEL_XOUT_H       = 0x2D,
    ICM20948_REG_ACCEL_XOUT_L       = 0x2E,
    ICM20948_REG_ACCEL_YOUT_H       = 0x2F,
    ICM20948_REG_ACCEL_YOUT_L       = 0x30,
    ICM20948_REG_ACCEL_ZOUT_H       = 0x31,
    ICM20948_REG_ACCEL_ZOUT_L       = 0x32,
    ICM20948_REG_GYRO_XOUT_H        = 0x33,
    ICM20948_REG_GYRO_XOUT_L        = 0x34,
    ICM20948_REG_GYRO_YOUT_H        = 0x35,
    ICM20948_REG_GYRO_YOUT_L        = 0x36,
    ICM20948_REG_GYRO_ZOUT_H        = 0x37,
    ICM20948_REG_GYRO_ZOUT_L        = 0x38,

    // Bank 1
    ICM20948_REG_SELF_TEST_X_GYRO   = 0x02,
    ICM20948_REG_SELF_TEST_Y_GYRO   = 0x03,
    ICM20948_REG_SELF_TEST_Z_GYRO   = 0x04,

    // Bank 2
    ICM20948_REG_GYRO_SMPLRT_DIV    = 0x00,
    ICM20948_REG_GYRO_CONFIG_1      = 0x01,
    ICM20948_REG_ACCEL_CONFIG       = 0x14,

    // Magnetometer (AK09916 over AUX I2C)
    AK09916_REG_WIA2                = 0x01,
    AK09916_REG_ST1                 = 0x10,
    AK09916_REG_HXL                 = 0x11,
    AK09916_REG_HXH                 = 0x12,
    AK09916_REG_HYL                 = 0x13,
    AK09916_REG_HYH                 = 0x14,
    AK09916_REG_HZL                 = 0x15,
    AK09916_REG_HZH                 = 0x16,
    AK09916_REG_ST2                 = 0x18,
    AK09916_REG_CNTL2               = 0x31,
    AK09916_REG_CNTL3               = 0x32

} icm20948_reg_t;

#endif // _ICM20948_REGS_H_
