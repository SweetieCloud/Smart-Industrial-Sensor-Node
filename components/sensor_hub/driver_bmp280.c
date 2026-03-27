#include "driver_bmp280.h"
#include "driver/i2c.h"
#include "bsp.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "BMP280";

#define BMP280_ADDR 0x76

#define BMP280_REG_CTRL_MEAS 0xF4
#define BMP280_REG_TEMP_MSB 0xFA
#define BMP280_REG_RESET 0xE0
#define BMP280_RESET_VALUE 0xB6

static uint16_t dig_T1;
static int16_t dig_T2, dig_T3;

esp_err_t driver_bmp280_init(void)
{
     esp_err_t err;
     uint8_t reset_cmd[2] = {BMP280_REG_RESET, BMP280_RESET_VALUE};
     i2c_master_write_to_device(I2C_MASTER_NUM, BMP280_ADDR, reset_cmd, 2, pdMS_TO_TICKS(100));

     ESP_LOGI(TAG, "Sent Software Reset command to BMP280");
     vTaskDelay(pdMS_TO_TICKS(300));
     uint8_t calib_data[6];
     err = i2c_master_write_read_device(I2C_MASTER_NUM, BMP280_ADDR, (uint8_t[]){0x88}, 1, calib_data, 6, pdMS_TO_TICKS(100));

     if (err != ESP_OK)
     {
          ESP_LOGE(TAG, "Failed to communicate with BMP280");
          return err;
     }

     dig_T1 = (calib_data[1] << 8) | calib_data[0];
     dig_T2 = (calib_data[3] << 8) | calib_data[2];
     dig_T3 = (calib_data[5] << 8) | calib_data[4];

     ESP_LOGI(TAG, "BMP280 Calib Data Loaded Successfully");
     return ESP_OK;
}

esp_err_t driver_bmp280_read_data(float *temperature, float *pressure)
{
     uint8_t raw_data[3];
     esp_err_t err;
     err = i2c_master_write_to_device(I2C_MASTER_NUM, BMP280_ADDR, (uint8_t[]){BMP280_REG_CTRL_MEAS, 0x25}, 2, pdMS_TO_TICKS(100));
     if (err != ESP_OK)
          return err;
     vTaskDelay(pdMS_TO_TICKS(10));

     err = i2c_master_write_read_device(I2C_MASTER_NUM, BMP280_ADDR, (uint8_t[]){BMP280_REG_TEMP_MSB}, 1, raw_data, 3, pdMS_TO_TICKS(100));

     if (err == ESP_OK)
     {
          int32_t adc_T = (raw_data[0] << 12) | (raw_data[1] << 4) | (raw_data[2] >> 4);

          if (adc_T == 0x80000)
          {
               return ESP_FAIL;
          }

          int32_t var1 = ((((adc_T >> 3) - ((int32_t)dig_T1 << 1))) * ((int32_t)dig_T2)) >> 11;
          int32_t var2 = (((((adc_T >> 4) - ((int32_t)dig_T1)) * ((adc_T >> 4) - ((int32_t)dig_T1))) >> 12) * ((int32_t)dig_T3)) >> 14;
          int32_t t_fine = var1 + var2;

          *temperature = (float)((t_fine * 5 + 128) >> 8) / 100.0f;
          *pressure = 0.0f;
     }
     return err;
}