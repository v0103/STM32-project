#pragma once

#include <stdint.h>

typedef struct {
  uint16_t therm_raw;
  uint16_t light_raw;
  uint16_t heat_level;
  uint16_t light_level;
} sensor_data_t;

void sensor_init(void);
sensor_data_t sensor_read(void);
