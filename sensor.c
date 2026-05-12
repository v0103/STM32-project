#include "sensor.h"
#include "adc.h"
#include "hal.h"

enum {
  SENSOR_THERM_PIN = PIN('A', 1),
  SENSOR_LIGHT_PIN = PIN('A', 2),
  SENSOR_THERM_CHANNEL = 1,
  SENSOR_LIGHT_CHANNEL = 2,
  SENSOR_ADC_MAX = 4095,
};

void sensor_init(void) {
  gpio_set_mode(SENSOR_THERM_PIN, GPIO_CFG_IN_ANALOG);
  gpio_set_mode(SENSOR_LIGHT_PIN, GPIO_CFG_IN_ANALOG);

  adc_init();
  adc_config_channel(SENSOR_THERM_CHANNEL);
  adc_config_channel(SENSOR_LIGHT_CHANNEL);
}

sensor_data_t sensor_read(void) {
  sensor_data_t data;

  data.therm_raw = adc_read(SENSOR_THERM_CHANNEL);
  data.light_raw = adc_read(SENSOR_LIGHT_CHANNEL);

  data.heat_level = SENSOR_ADC_MAX - data.therm_raw;
  data.light_level = SENSOR_ADC_MAX - data.light_raw;

  return data;
}
