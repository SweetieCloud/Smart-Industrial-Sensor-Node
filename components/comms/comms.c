#include "comms.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "nvs_flash.h"
#include <string.h>
#include "esp_sntp.h"
#include <time.h>
#include <sys/time.h>
#include "storage.h"
#include "wifi_provisioning/manager.h"
#include "wifi_provisioning/scheme_softap.h"

static const char *TAG = "COMMS";

#define MQTT_BROKER_URI "mqtt://broker.emqx.io"
#define MQTT_TOPIC "v1/devices/me/telemetry"
static esp_mqtt_client_handle_t mqtt_client = NULL;
bool is_mqtt_connected = false;

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
     switch ((esp_mqtt_event_id_t)event_id)
     {
     case MQTT_EVENT_CONNECTED:
          ESP_LOGI(TAG, "MQTT Connected to Broker!");
          is_mqtt_connected = true;
          storage_upload_offline_data();
          break;
     case MQTT_EVENT_DISCONNECTED:
          ESP_LOGW(TAG, "MQTT Disconnected!");
          is_mqtt_connected = false;
          break;
     default:
          break;
     }
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
     if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
     {
          esp_wifi_connect();
     }
     else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
     {
          ESP_LOGW(TAG, "WiFi Lost! Retrying in 3 seconds...");
          vTaskDelay(pdMS_TO_TICKS(3000));
          esp_wifi_connect();
     }
     else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
     {
          ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
          ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
          esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
          esp_sntp_setservername(0, "pool.ntp.org");
          esp_sntp_init();
          setenv("TZ", "ICT-7", 1);
          tzset();
          esp_mqtt_client_start(mqtt_client);
     }
}

void comms_init(void)
{
     esp_netif_init();
     esp_event_loop_create_default();
     esp_netif_create_default_wifi_sta();
     esp_netif_create_default_wifi_ap();
     wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
     esp_wifi_init(&cfg);

     esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL);
     esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL);

     // ========================================================
     // KIẾN TRÚC SOFTAP PROVISIONING
     // ========================================================
     wifi_prov_mgr_config_t config = {
         .scheme = wifi_prov_scheme_softap,
         .scheme_event_handler = WIFI_PROV_EVENT_HANDLER_NONE};
     ESP_ERROR_CHECK(wifi_prov_mgr_init(config));

     bool provisioned = false;
     ESP_ERROR_CHECK(wifi_prov_mgr_is_provisioned(&provisioned));

     if (!provisioned)
     {
          ESP_LOGI(TAG, "NO WIFI AVAILABLE! SoftAP is currently being used: PROV_SMART_NODE");
          esp_wifi_set_mode(WIFI_MODE_APSTA);
          ESP_ERROR_CHECK(wifi_prov_mgr_start_provisioning(WIFI_PROV_SECURITY_1, "12345678", "PROV_SMART_NODE", NULL));
     }
     else
     {
          ESP_LOGI(TAG, "WiFi information is now in memory! Turn off the router and try connecting....");
          wifi_prov_mgr_deinit();
          esp_wifi_set_mode(WIFI_MODE_STA);
          esp_wifi_start();
     }

     esp_mqtt_client_config_t mqtt_cfg = {
         .broker.address.uri = "mqtt://thingsboard.cloud",
         .credentials.username = "TFNk19OYfSKcyr4dqMdr",
     };
     mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
     esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
}

void comms_publish_sensor_data(float current, float vibration, float temp, time_t timestamp)
{
     struct tm timeinfo;
     localtime_r(&timestamp, &timeinfo);
     char time_str[64];
     strftime(time_str, sizeof(time_str), "%Y-%m-%dT%H:%M:%S", &timeinfo);

     char payload[200];
     snprintf(payload, sizeof(payload),
              "{\"current\":%.2f, \"vibration\":%.3f, \"temperature\":%.2f}",
              current, vibration, temp);

     // MẤT MẠNG -> GHI VÀO SPIFFS
     if (!is_mqtt_connected)
     {
          ESP_LOGW(TAG, "MQTT Offline -> Temporarily saved to flash memory.");
          storage_save_offline_data(payload);
          return;
     }
     esp_mqtt_client_publish(mqtt_client, MQTT_TOPIC, payload, 0, 0, 0);
     int msg_id = esp_mqtt_client_publish(mqtt_client, MQTT_TOPIC, payload, 0, 0, 0);
     ESP_LOGI(TAG, "Sent to ThingsBoard (Topic: %s) -> %s", MQTT_TOPIC, payload);
}
void comms_publish_raw(const char *payload)
{
     if (is_mqtt_connected)
     {
          esp_mqtt_client_publish(mqtt_client, MQTT_TOPIC, payload, 0, 0, 0);
     }
}