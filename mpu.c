#include "mpu.h"
#include "i2c.h"
#include <stddef.h>
enum {
  MPU_ADDR = 0x68,

  MPU_REG_WHO_AM_I = 0x75,
  MPU_REG_PWR_MGMT_1 = 0x6B,

  MPU_WHO_MPU6050 = 0x68,
  MPU_WHO_MPU6500 = 0x70,
};

bool mpu_read_reg(uint8_t reg, uint8_t *value) {
  if (value == NULL) {
    return false;
  }

  return i2c_write_read(MPU_ADDR, &reg, 1, value, 1);
}

bool mpu_write_reg(uint8_t reg, uint8_t value) {
  uint8_t data[2] = {reg, value};

  return i2c_write(MPU_ADDR, data, 2);
}

bool mpu_read_regs(uint8_t reg, uint8_t *data, uint16_t len) {
  if (len > 0 && data == NULL) {
    return false;
  }

  return i2c_write_read(MPU_ADDR, &reg, 1, data, len);
}

bool mpu_init(void) {
  uint8_t who = 0;

  if (!mpu_read_reg(MPU_REG_WHO_AM_I, &who)) {
    return false;
  }

  if (who != MPU_WHO_MPU6050 && who != MPU_WHO_MPU6500) {
    return false;
  }

  if (!mpu_write_reg(MPU_REG_PWR_MGMT_1, 0x00)) {
    return false;
  }

  return true;
}