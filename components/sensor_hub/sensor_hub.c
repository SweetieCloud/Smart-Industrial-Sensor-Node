#include "sensor_hub.h"
#include "bsp.h"
#include "driver_mpu6050.h"
#include "driver_bmp280.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include <math.h>
#include "driver/i2c.h"

static const char *TAG = "SENSOR_HUB";

QueueHandle_t sensor_data_queue = NULL;

#define NUM_SAMPLES 20
static int acs_samples[NUM_SAMPLES];
static int sample_index = 0;
static int sample_sum = 0;

#define ACS712_ZERO_OFFSET -4.66f

static int moving_average_filter(int new_sample)
{
     sample_sum -= acs_samples[sample_index];
     acs_samples[sample_index] = new_sample;
     sample_sum += acs_samples[sample_index];
     sample_index = (sample_index + 1) % NUM_SAMPLES;
     return (sample_sum / NUM_SAMPLES);
}

static void sensor_task(void *pvParameters)
{
     int raw_adc = 0;
     int filtered_adc = 0;
     float ax, ay, az;

     for (int i = 0; i < NUM_SAMPLES; i++)
          acs_samples[i] = 0;

     driver_mpu6050_init();
     driver_bmp280_init();

     TickType_t xLastWakeTime = xTaskGetTickCount();
     const TickType_t xFrequency = pdMS_TO_TICKS(100);

     while (1)
     {
          sensor_data_t current_data = {0};
          if (adc1_handle != NULL)
          {
               ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC_ACS712_CHAN, &raw_adc));
               filtered_adc = moving_average_filter(raw_adc);
               float v_adc = ((float)filtered_adc / 4095.0f) * 3.3f;
               float v_out_acs = v_adc * 1.5f;
               float raw_current = (v_out_acs - 2.5f) / 0.1f;

               current_data.current = raw_current - ACS712_ZERO_OFFSET;
               if (current_data.current < 0.1f && current_data.current > -0.1f)
               {
                    current_data.current = 0.0f;
               }
          }

          if (driver_mpu6050_read_accel(&ax, &ay, &az) == ESP_OK)
          {
               float a_total = sqrtf(ax * ax + ay * ay + az * az);
               current_data.vibration = fabsf(a_total - 1.0f);
               if (current_data.vibration < 0.10f)
                    current_data.vibration = 0.0f;
          }
          else
          {
               current_data.vibration = -1.0f;
          }

          if (driver_bmp280_read_data(&current_data.temperature, &current_data.pressure) != ESP_OK)
          {
               ESP_LOGW(TAG, "Error BMP280! Check I2C.");
               current_data.temperature = -99.0f;
          }
          time(&current_data.timestamp);

          if (sensor_data_queue != NULL)
          {
               xQueueSend(sensor_data_queue, &current_data, 0);
          }

          vTaskDelayUntil(&xLastWakeTime, xFrequency);
     }
}

void i2c_scanner(void)
{
     ESP_LOGI(TAG, ">> Start scanning the I2C Bus...");
     uint8_t address;
     int devices_found = 0;

     for (address = 1; address < 127; address++)
     {
          esp_err_t err = i2c_master_write_to_device(I2C_MASTER_NUM, address, NULL, 0, pdMS_TO_TICKS(100));

          if (err == ESP_OK)
          {
               ESP_LOGI(TAG, "   -> The device was found at this address: 0x%02X", address);
               devices_found++;
          }
          else if (err == ESP_ERR_TIMEOUT)
          {
               // Timeout có nghĩa là không có thiết bị ở địa chỉ này
          }
     }

     if (devices_found == 0)
     {
          ESP_LOGW(TAG, ">> No I2C devices found!");
     }
     else
     {
          ESP_LOGI(TAG, ">> Scan complete. Total number of devices: %d", devices_found);
     }
}

void sensor_hub_init(void)
{
     sensor_data_queue = xQueueCreate(10, sizeof(sensor_data_t));

     xTaskCreate(sensor_task, "sensor_task", 4096, NULL, 4, NULL);
}