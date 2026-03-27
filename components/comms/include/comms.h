#ifndef COMMS_H
#define COMMS_H
#include <time.h>
#include <stdbool.h>

void comms_init(void);
void comms_publish_sensor_data(float current, float vibration, float temp, time_t timestamp);
void comms_publish_raw(const char *payload);
#endif