idf_component_register(SRCS "comms.c"
                       INCLUDE_DIRS "include"
                       REQUIRES esp_wifi mqtt storage wifi_provisioning nvs_flash)