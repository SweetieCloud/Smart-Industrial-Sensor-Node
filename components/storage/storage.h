#ifndef STORAGE_H
#define STORAGE_H

#include "esp_err.h"

esp_err_t storage_init(void);
esp_err_t storage_save_offline_data(const char *json_payload);
void storage_upload_offline_data(void);

#endif // STORAGE_H