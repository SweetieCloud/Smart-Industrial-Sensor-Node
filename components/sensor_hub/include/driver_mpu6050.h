#ifndef DRIVER_MPU6050_H
#define DRIVER_MPU6050_H

#include "esp_err.h"

esp_err_t driver_mpu6050_init(void);
esp_err_t driver_mpu6050_read_accel(float *accel_x, float *accel_y, float *accel_z);

#endif // DRIVER_MPU6050_H