#ifndef DRIVER_BMP280_H
#define DRIVER_BMP280_H

#include "esp_err.h"

esp_err_t driver_bmp280_init(void);
esp_err_t driver_bmp280_read_data(float *temperature, float *pressure);

#endif // DRIVER_BMP280_H