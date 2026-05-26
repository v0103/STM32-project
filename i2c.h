#pragma once

#include <stdbool.h>
#include <stdint.h>

void soft_i2c_init(void);
bool i2c_probe(uint8_t addr);
bool i2c_write(uint8_t addr, const uint8_t *data, uint16_t len);
bool i2c_read(uint8_t addr, uint8_t *data, uint16_t len);
bool i2c_write_read(uint8_t addr,
                    const uint8_t *tx, uint16_t tx_len,
                    uint8_t *rx, uint16_t rx_len);