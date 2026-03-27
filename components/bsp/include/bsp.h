#ifndef BSP_H
#define BSP_H

#include "esp_err.h"
#include "esp_adc/adc_oneshot.h"
#include "driver/gpio.h"

#define I2C_MASTER_SCL_IO 22
#define I2C_MASTER_SDA_IO 21
#define I2C_MASTER_NUM I2C_NUM_0
#define I2C_MASTER_FREQ_HZ 100000

#define ADC_ACS712_CHAN ADC_CHANNEL_6
extern adc_oneshot_unit_handle_t adc1_handle;

esp_err_t bsp_i2c_init(void);
esp_err_t bsp_adc_init(void);

#define BUZZER_PIN 19

extern adc_oneshot_unit_handle_t adc1_handle;

esp_err_t bsp_i2c_init(void);
esp_err_t bsp_adc_init(void);

void bsp_buzzer_init(void);
void bsp_buzzer_set(bool on);

#endif