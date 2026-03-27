#include "storage.h"
#include "esp_spiffs.h"
#include "esp_log.h"
#include <string.h>
#include <sys/stat.h>
#include "comms.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "STORAGE";

esp_err_t storage_init(void)
{
     ESP_LOGI(TAG, "Initializing the drive SPIFFS...");

     esp_vfs_spiffs_conf_t conf = {
         .base_path = "/spiffs",
         .partition_label = NULL,
         .max_files = 5,
         .format_if_mount_failed = true};

     esp_err_t ret = esp_vfs_spiffs_register(&conf);

     if (ret != ESP_OK)
     {
          if (ret == ESP_FAIL)
               ESP_LOGE(TAG, "Failed to mount or format filesystem");
          else if (ret == ESP_ERR_NOT_FOUND)
               ESP_LOGE(TAG, "Failed to find SPIFFS partition");
          else
               ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
          return ret;
     }

     size_t total = 0, used = 0;
     ret = esp_spiffs_info(NULL, &total, &used);
     if (ret == ESP_OK)
     {
          ESP_LOGI(TAG, "SPIFFS is working! Total capacity: %d bytes, Used: %d bytes", total, used);
     }
     else
     {
          ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
     }

     return ESP_OK;
}

esp_err_t storage_save_offline_data(const char *json_payload)
{
     struct stat st;
     if (stat("/spiffs/offline_data.txt", &st) == 0)
     {
          if (st.st_size > 500 * 1024)
          {
               ESP_LOGW(TAG, "⚠️Offline file is too large (%ld bytes). Delete it to save memory!", st.st_size);
               remove("/spiffs/offline_data.txt");
          }
     }

     FILE *f = fopen("/spiffs/offline_data.txt", "a");
     if (f == NULL)
     {
          ESP_LOGE(TAG, "Error! Unable to open file for writing.");
          return ESP_FAIL;
     }

     fprintf(f, "%s\n", json_payload);
     fclose(f);

     ESP_LOGI(TAG, "Saved Offline -> %s", json_payload);
     return ESP_OK;
}

static void offline_upload_task(void *pvParameters)
{
     FILE *f = fopen("/spiffs/offline_data.txt", "r");
     if (f == NULL)
     {
          ESP_LOGI(TAG, "There is no offline data available to send as compensation.");
          vTaskDelete(NULL);
     }

     ESP_LOGI(TAG, "Start sending offline data compensation to the Cloud...");
     char line[256];

     while (fgets(line, sizeof(line), f) != NULL)
     {
          line[strcspn(line, "\n")] = 0;
          comms_publish_raw(line);
          ESP_LOGI(TAG, "Resume the upload: %s", line);
          vTaskDelay(pdMS_TO_TICKS(100));
     }

     fclose(f);
     remove("/spiffs/offline_data.txt");
     ESP_LOGI(TAG, "The Offline file was deleted after uploading.");

     vTaskDelete(NULL);
}

void storage_upload_offline_data(void)
{
     xTaskCreate(offline_upload_task, "offline_upload", 4096, NULL, 2, NULL);
}