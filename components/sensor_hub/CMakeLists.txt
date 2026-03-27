idf_component_register(SRCS "sensor_hub.c" "driver_mpu6050.c" "driver_bmp280.c"
                       INCLUDE_DIRS "include"
                       REQUIRES bsp)