
idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES bsp sensor_hub comms nvs_flash storage)