#include "mpu.h"
#include "i2c.h"
#include <stddef.h>
enum {
  MPU_ADDR = 0x68,

  MPU_REG_WHO_AM_I = 0x75,
  MPU_REG_PWR_MGMT_1 = 0x6B,
  MPU_REG_GYRO_CONFIG = 0x1B,
  MPU_REG_ACCEL_CONFIG = 0x1C,
  MPU_REG_ACCEL_XOUT_H = 0x3B,
  MPU_REG_GYRO_XOUT_H = 0x43,

  MPU_WHO_MPU6050 = 0x68,
  MPU_WHO_MPU6500 = 0x70,

  MPU_ACCEL_LSB_PER_G = 16384,
  MPU_GYRO_LSB_PER_DPS = 131,
};

static mpu_gyro_t s_gyro_bias;
static bool s_gyro_calibrated;

static int16_t mpu_make_i16(uint8_t high, uint8_t low) {
  return (int16_t)(((uint16_t) high << 8) | low);
}

static int16_t mpu_i32_to_i16(int32_t value) {
  if (value > INT16_MAX) {
    return INT16_MAX;
  }

  if (value < INT16_MIN) {
    return INT16_MIN;
  }

  return (int16_t) value;
}

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
  uint8_t pwr = 0;

  if (!mpu_read_reg(MPU_REG_WHO_AM_I, &who)) {
    return false;
  }

  if (who != MPU_WHO_MPU6050 && who != MPU_WHO_MPU6500) {
    return false;
  }

  if (!mpu_write_reg(MPU_REG_PWR_MGMT_1, 0x00)) {
    return false;
  }

  if (!mpu_read_reg(MPU_REG_PWR_MGMT_1, &pwr)) {
    return false;
  }

  if ((pwr & 0x40U) != 0) {
    return false;
  }

  if (!mpu_write_reg(MPU_REG_ACCEL_CONFIG, 0x00)) {
    return false;
  }

  if (!mpu_write_reg(MPU_REG_GYRO_CONFIG, 0x00)) {
    return false;
  }

  return true;
}

bool mpu_read_accel_raw(mpu_accel_t *accel) {
  uint8_t data[6];

  if (accel == NULL) {
    return false;
  }

  if (!mpu_read_regs(MPU_REG_ACCEL_XOUT_H, data, 6)) {
    return false;
  }

  accel->x = mpu_make_i16(data[0], data[1]);
  accel->y = mpu_make_i16(data[2], data[3]);
  accel->z = mpu_make_i16(data[4], data[5]);

  return true;
}

bool mpu_read_gyro_raw(mpu_gyro_t *gyro) {
  uint8_t data[6];

  if (gyro == NULL) {
    return false;
  }

  if (!mpu_read_regs(MPU_REG_GYRO_XOUT_H, data, 6)) {
    return false;
  }

  gyro->x = mpu_make_i16(data[0], data[1]);
  gyro->y = mpu_make_i16(data[2], data[3]);
  gyro->z = mpu_make_i16(data[4], data[5]);

  return true;
}

bool mpu_calibrate_gyro(uint16_t samples) {
  int32_t sum_x = 0;
  int32_t sum_y = 0;
  int32_t sum_z = 0;

  if (samples == 0) {
    return false;
  }

  for (uint16_t i = 0; i < samples; i++) {
    mpu_gyro_t gyro;

    if (!mpu_read_gyro_raw(&gyro)) {
      return false;
    }

    sum_x += gyro.x;
    sum_y += gyro.y;
    sum_z += gyro.z;
  }

  s_gyro_bias.x = mpu_i32_to_i16(sum_x / samples);
  s_gyro_bias.y = mpu_i32_to_i16(sum_y / samples);
  s_gyro_bias.z = mpu_i32_to_i16(sum_z / samples);
  s_gyro_calibrated = true;

  return true;
}

bool mpu_read_motion_raw(mpu_motion_t *motion) {
  uint8_t data[14];

  if (motion == NULL) {
    return false;
  }

  if (!mpu_read_regs(MPU_REG_ACCEL_XOUT_H, data, 14)) {
    return false;
  }

  motion->accel.x = mpu_make_i16(data[0], data[1]);
  motion->accel.y = mpu_make_i16(data[2], data[3]);
  motion->accel.z = mpu_make_i16(data[4], data[5]);
  motion->temp_raw = mpu_make_i16(data[6], data[7]);
  motion->gyro.x = mpu_make_i16(data[8], data[9]);
  motion->gyro.y = mpu_make_i16(data[10], data[11]);
  motion->gyro.z = mpu_make_i16(data[12], data[13]);

  return true;
}

static int32_t accel_raw_to_mg(int16_t raw) {
  return ((int32_t) raw * 1000) / MPU_ACCEL_LSB_PER_G;
}

static int32_t gyro_raw_to_mdps(int16_t raw) {
  return ((int32_t) raw * 1000) / MPU_GYRO_LSB_PER_DPS;
}

static int16_t gyro_remove_bias(int16_t raw, int16_t bias) {
  if (!s_gyro_calibrated) {
    return raw;
  }

  return mpu_i32_to_i16((int32_t) raw - bias);
}

bool mpu_scale_motion(const mpu_motion_t *raw, mpu_motion_scaled_t *scaled) {
  if (raw == NULL || scaled == NULL) {
    return false;
  }

  scaled->accel.x_mg = accel_raw_to_mg(raw->accel.x);
  scaled->accel.y_mg = accel_raw_to_mg(raw->accel.y);
  scaled->accel.z_mg = accel_raw_to_mg(raw->accel.z);
  scaled->temp_raw = raw->temp_raw;
  scaled->gyro.x_mdps = gyro_raw_to_mdps(gyro_remove_bias(raw->gyro.x, s_gyro_bias.x));
  scaled->gyro.y_mdps = gyro_raw_to_mdps(gyro_remove_bias(raw->gyro.y, s_gyro_bias.y));
  scaled->gyro.z_mdps = gyro_raw_to_mdps(gyro_remove_bias(raw->gyro.z, s_gyro_bias.z));

  return true;
}

bool mpu_read_motion_scaled(mpu_motion_scaled_t *motion) {
  mpu_motion_t raw;

  if (motion == NULL) {
    return false;
  }

  if (!mpu_read_motion_raw(&raw)) {
    return false;
  }

  return mpu_scale_motion(&raw, motion);
}
