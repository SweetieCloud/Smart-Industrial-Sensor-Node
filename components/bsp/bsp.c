#include "bsp.h"
#include "driver/i2c.h"
#include "esp_log.h"

static const char *TAG = "BSP";
adc_oneshot_unit_handle_t adc1_handle = NULL;

esp_err_t bsp_i2c_init(void)
{
     int i2c_master_port = I2C_MASTER_NUM;
     gpio_config_t io_conf = {
         .intr_type = GPIO_INTR_DISABLE,
         .mode = GPIO_MODE_OUTPUT_OD,
         .pin_bit_mask = (1ULL << I2C_MASTER_SDA_IO) | (1ULL << I2C_MASTER_SCL_IO),
         .pull_down_en = 0,
         .pull_up_en = 1};
     gpio_config(&io_conf);

     for (int i = 0; i < 9; i++)
     {
          gpio_set_level(I2C_MASTER_SCL_IO, 0);
          vTaskDelay(pdMS_TO_TICKS(1));
          gpio_set_level(I2C_MASTER_SCL_IO, 1);
          vTaskDelay(pdMS_TO_TICKS(1));
     }

     gpio_set_direction(I2C_MASTER_SDA_IO, GPIO_MODE_INPUT);
     gpio_set_direction(I2C_MASTER_SCL_IO, GPIO_MODE_INPUT);
     vTaskDelay(pdMS_TO_TICKS(10));

     i2c_config_t conf = {
         .mode = I2C_MODE_MASTER,
         .sda_io_num = I2C_MASTER_SDA_IO,
         .scl_io_num = I2C_MASTER_SCL_IO,
         .sda_pullup_en = GPIO_PULLUP_ENABLE,
         .scl_pullup_en = GPIO_PULLUP_ENABLE,
         .master.clk_speed = I2C_MASTER_FREQ_HZ,
     };

     i2c_param_config(i2c_master_port, &conf);

     esp_err_t err = i2c_driver_install(i2c_master_port, conf.mode, 0, 0, 0);
     if (err == ESP_OK)
     {
          ESP_LOGI(TAG, "I2C Master initialized successfully");
     }
     else
     {
          ESP_LOGE(TAG, "I2C Master initialization failed!");
     }
     return err;
}

esp_err_t bsp_adc_init(void)
{

     adc_oneshot_unit_init_cfg_t init_config1 = {
         .unit_id = ADC_UNIT_1,
         .ulp_mode = ADC_ULP_MODE_DISABLE,
     };
     ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

     adc_oneshot_chan_cfg_t config = {
         .bitwidth = ADC_BITWIDTH_DEFAULT,
         .atten = ADC_ATTEN_DB_12,
     };
     ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_ACS712_CHAN, &config));

     ESP_LOGI(TAG, "ADC1 Channel %d initialized for ACS712", ADC_ACS712_CHAN);
     return ESP_OK;
}

void bsp_buzzer_init(void)
{
     gpio_config_t io_conf = {
         .intr_type = GPIO_INTR_DISABLE,
         .mode = GPIO_MODE_OUTPUT,
         .pin_bit_mask = (1ULL << BUZZER_PIN),
         .pull_down_en = 0,
         .pull_up_en = 0};
     gpio_config(&io_conf);
     gpio_set_level(BUZZER_PIN, 1);
     ESP_LOGI(TAG, "Buzzer initialized on GPIO %d", BUZZER_PIN);
}

void bsp_buzzer_set(bool on)
{
     gpio_set_level(BUZZER_PIN, on ? 0 : 1);
}