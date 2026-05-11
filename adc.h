#pragma once

#include <stdint.h>

void adc_init(void);
void adc_config_channel(uint8_t channel);
uint16_t adc_read(uint8_t channel);
