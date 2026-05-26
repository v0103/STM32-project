#pragma once

#include <stdbool.h>
#include <stdint.h>

bool mpu_read_reg(uint8_t reg, uint8_t *value);
bool mpu_write_reg(uint8_t reg, uint8_t value);
bool mpu_read_regs(uint8_t reg, uint8_t *data, uint16_t len);
bool mpu_init(void);