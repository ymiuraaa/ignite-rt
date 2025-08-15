#pragma once

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float ax, ay, az;     // Accel in m/s^2
    float gx, gy, gz;     // Gyro in deg/s
} imu_data_t;

esp_err_t imu_init(void);
void imu_task(void *arg);

bool get_imu_data(imu_data_t *out);

#ifdef __cplusplus
}
#endif