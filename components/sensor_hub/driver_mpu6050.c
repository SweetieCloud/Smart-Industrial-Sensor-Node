#include "driver_mpu6050.h"
#include "driver/i2c.h"
#include "bsp.h"
#include "esp_log.h"

static const char *TAG = "MPU6050";

#define MPU6050_ADDR 0x68
#define MPU6050_PWR_MGMT_1 0x6B
#define MPU6050_ACCEL_XOUT_H 0x3B

esp_err_t driver_mpu6050_init(void)
{
     uint8_t write_buf[2] = {MPU6050_PWR_MGMT_1, 0x00};

     esp_err_t err = i2c_master_write_to_device(I2C_MASTER_NUM, MPU6050_ADDR, write_buf, sizeof(write_buf), pdMS_TO_TICKS(100));

     if (err == ESP_OK)
     {
          ESP_LOGI(TAG, "MPU6050 woken up and initialized successfully");
     }
     else
     {
          ESP_LOGE(TAG, "Failed to initialize MPU6050 (Check wiring)");
     }
     return err;
}

esp_err_t driver_mpu6050_read_accel(float *accel_x, float *accel_y, float *accel_z)
{
     uint8_t raw_data[6];

     esp_err_t err = i2c_master_write_read_device(I2C_MASTER_NUM, MPU6050_ADDR,
                                                  (uint8_t[]){MPU6050_ACCEL_XOUT_H}, 1,
                                                  raw_data, sizeof(raw_data), pdMS_TO_TICKS(100));
     if (err == ESP_OK)
     {
          int16_t raw_x = (raw_data[0] << 8) | raw_data[1];
          int16_t raw_y = (raw_data[2] << 8) | raw_data[3];
          int16_t raw_z = (raw_data[4] << 8) | raw_data[5];

          *accel_x = (float)raw_x / 16384.0f;
          *accel_y = (float)raw_y / 16384.0f;
          *accel_z = (float)raw_z / 16384.0f;
     }
     return err;
}