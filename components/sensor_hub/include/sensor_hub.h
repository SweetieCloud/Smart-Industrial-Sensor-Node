#ifndef SENSOR_HUB_H
#define SENSOR_HUB_H
#include <time.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

typedef struct
{
     float temperature;
     float pressure;
     float current;
     float vibration;
     time_t timestamp;
} sensor_data_t;

extern QueueHandle_t sensor_data_queue;

void sensor_hub_init(void);

#endif // SENSOR_HUB_H