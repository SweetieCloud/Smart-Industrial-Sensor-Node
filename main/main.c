#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "bsp.h"
#include "sensor_hub.h"
#include "comms.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "storage.h"
extern bool is_mqtt_connected;
static const char *TAG = "MAIN";

#define ALARM_TEMP_THRESHOLD 34.5f
#define ALARM_VIB_THRESHOLD 0.5f

void data_logger_task(void *pvParameters)
{
     sensor_data_t received_data;
     bool is_alarming = false;
     uint8_t publish_counter = 0;
     float peak_vibration = 0.0f;
     float peak_current = 0.0f;
     float peak_temperature = 0.0f;

     while (1)
     {
          if (xQueueReceive(sensor_data_queue, &received_data, portMAX_DELAY) == pdPASS)
          {
               printf(">Vib:%.3f\n", received_data.vibration);
               printf(">Temp:%.2f\n", received_data.temperature);
               printf(">Current:%.2f\n", received_data.current);
               fflush(stdout);
               if (received_data.vibration > peak_vibration)
                    peak_vibration = received_data.vibration;
               if (received_data.current > peak_current)
                    peak_current = received_data.current;
               if (received_data.temperature > peak_temperature)
                    peak_temperature = received_data.temperature;

               bool force_publish = false;

               if (received_data.temperature > ALARM_TEMP_THRESHOLD ||
                   received_data.vibration > ALARM_VIB_THRESHOLD)
               {
                    if (!is_alarming)
                    {
                         ESP_LOGE(TAG, "⚠️ ALARM TRIGGERED!");
                         bsp_buzzer_set(true);
                         is_alarming = true;
                         force_publish = true;
                    }
               }
               else
               {
                    if (is_alarming)
                    {
                         ESP_LOGI(TAG, "✅ ALARM CLEARED!");
                         bsp_buzzer_set(false);
                         is_alarming = false;
                         force_publish = true;
                    }
               }

               publish_counter++;
               uint8_t target_cycles = is_mqtt_connected ? 5 : 20;

               if (publish_counter >= target_cycles || force_publish)
               {
                    comms_publish_sensor_data(peak_current, peak_vibration, peak_temperature, time(NULL));

                    publish_counter = 0;
                    peak_vibration = received_data.vibration;
                    peak_current = received_data.current;
                    peak_temperature = received_data.temperature;
               }
          }
     }
}

void app_main(void)
{
     WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

     ESP_LOGI(TAG, "--- SMART INDUSTRIAL SENSOR NODE ---");

     esp_err_t ret = nvs_flash_init();
     if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
     {
          ESP_ERROR_CHECK(nvs_flash_erase());
          ret = nvs_flash_init();
     }
     ESP_ERROR_CHECK(ret);
     storage_init();
     bsp_adc_init();
     bsp_buzzer_init();
     bsp_buzzer_set(true);
     vTaskDelay(pdMS_TO_TICKS(100));
     bsp_buzzer_set(false);

     ESP_LOGI(TAG, "WiFi is on, prepare for a voltage drop....");
     comms_init();
     ESP_LOGI(TAG, "Waiting for WiFi to stabilize (5s)...");
     vTaskDelay(pdMS_TO_TICKS(5000));

     ESP_LOGI(TAG, "The power supply is stable. I2C Sensor startup!");
     bsp_i2c_init();
     sensor_hub_init();

     xTaskCreate(data_logger_task, "logger_task", 4096, NULL, 3, NULL);
     ESP_LOGI(TAG, "System Running Normally.");

     while (1)
     {
          vTaskDelay(pdMS_TO_TICKS(10000));
     }
}