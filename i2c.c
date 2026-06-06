#include "i2c.h"
#include "hal.h"

enum {
  SOFT_I2C_SCL = PIN('B', 8),
  SOFT_I2C_SDA = PIN('B', 9),
};

static void i2c_delay(void) {
  spin(100);
}

static void scl_low(void) {
  gpio_write(SOFT_I2C_SCL, false);
}

static void scl_release(void) {
  gpio_write(SOFT_I2C_SCL, true);
}

static void sda_low(void) {
  gpio_write(SOFT_I2C_SDA, false);
}

static void sda_release(void) {
  gpio_write(SOFT_I2C_SDA, true);
}

static bool sda_read(void) {
  return gpio_read(SOFT_I2C_SDA);
}

void soft_i2c_init(void) {
  gpio_set_mode(SOFT_I2C_SCL, GPIO_CFG_OUT_OD_2MHZ);
  gpio_set_mode(SOFT_I2C_SDA, GPIO_CFG_OUT_OD_2MHZ);

  scl_release();
  sda_release();
}
static void i2c_start(void){
    sda_release();
    scl_release();
    i2c_delay();

    sda_low();
    i2c_delay();

    scl_low();
    i2c_delay();
}
static void i2c_stop(void){
    sda_low();
    i2c_delay();

    scl_release();
    i2c_delay();

    sda_release();
    i2c_delay();
}
static bool i2c_read_ack(void) {
  bool ack;

  sda_release();
  i2c_delay();

  scl_release();
  i2c_delay();

  ack = !sda_read(); // true if slave pulled SDA low for ACK

  scl_low();
  i2c_delay();

  return ack;
}
static void i2c_write_bit(bool bit) {
  scl_low();

  if (bit) {
    sda_release();
  } else {
    sda_low();
  }

  i2c_delay();

  scl_release();
  i2c_delay();

  scl_low();
  i2c_delay();
}

static bool i2c_write_byte(uint8_t byte) {
  for (int bit = 7; bit >= 0; bit--) {
    i2c_write_bit((byte & (1U << bit)) != 0);
  }

  return i2c_read_ack();
}

bool i2c_write(uint8_t addr, const uint8_t *data, uint16_t len) {
  if (len > 0 && data == NULL) {
    return false;
  }
  i2c_start();

  if(!i2c_write_byte((uint8_t)(addr << 1))){
    i2c_stop();
    return false;
  }
  for(uint16_t i = 0; i < len; i++){
    if(!i2c_write_byte(*data++)){
      i2c_stop();
      return false;
    }
  }
  i2c_stop();
  return true;
}

bool i2c_probe(uint8_t addr) {
  return i2c_write(addr, NULL, 0);
}

static bool i2c_read_bit(void) {
  bool bit;

  sda_release();
  i2c_delay();

  scl_release();
  i2c_delay();

  bit = sda_read(); // true if bit is high

  scl_low();
  i2c_delay();

  return bit;
}
static uint8_t i2c_read_byte(bool ack) {
  uint8_t byte = 0;

  for (int bit = 0; bit < 8; bit++) {
    byte <<= 1;
    if (i2c_read_bit()) {
      byte |= 1U;
    }
  }
  if(ack){
    sda_low();
  }else{
    sda_release();
  }

  i2c_delay();

  scl_release();
  i2c_delay();

  scl_low();
  i2c_delay();

  sda_release();
  return byte;
}

bool i2c_read(uint8_t addr, uint8_t *data, uint16_t len) {
  if (len > 0 && data == NULL) {
    return false;
  }

  i2c_start();

  if (!i2c_write_byte((uint8_t)((addr << 1) | 1U))) {
    i2c_stop();
    return false;
  }

  for (uint16_t i = 0; i < len; i++) {
    bool ack = (i + 1U) < len;
    data[i] = i2c_read_byte(ack);
  }

  i2c_stop();
  return true;
}

bool i2c_write_read(uint8_t addr,
                    const uint8_t *tx, uint16_t tx_len,
                    uint8_t *rx, uint16_t rx_len) {
  if (tx_len > 0 && tx == NULL) {
    return false;
  }

  if (rx_len > 0 && rx == NULL) {
    return false;
  }

  i2c_start();

  if (!i2c_write_byte((uint8_t)(addr << 1))) {
    i2c_stop();
    return false;
  }

  for (uint16_t i = 0; i < tx_len; i++) {
    if (!i2c_write_byte(tx[i])) {
      i2c_stop();
      return false;
    }
  }

  i2c_start();  // repeated START

  if (!i2c_write_byte((uint8_t)((addr << 1) | 1U))) {
    i2c_stop();
    return false;
  }

  for (uint16_t i = 0; i < rx_len; i++) {
    bool ack = (i + 1U) < rx_len;
    rx[i] = i2c_read_byte(ack);
  }

  i2c_stop();
  return true;
}