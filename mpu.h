#pragma once

#include <stdbool.h>
#include <stdint.h>

bool mpu_read_reg(uint8_t reg, uint8_t *value);
bool mpu_write_reg(uint8_t reg, uint8_t value);
bool mpu_read_regs(uint8_t reg, uint8_t *data, uint16_t len);
bool mpu_init(void);

typedef struct {
  int16_t x;
  int16_t y;
  int16_t z;
} mpu_accel_t;

bool mpu_read_accel_raw(mpu_accel_t *accel);

typedef struct {
  int16_t x;
  int16_t y;
  int16_t z;
} mpu_gyro_t;

bool mpu_read_gyro_raw(mpu_gyro_t *gyro);
bool mpu_calibrate_gyro(uint16_t samples);

typedef struct {
  mpu_accel_t accel;
  int16_t temp_raw;
  mpu_gyro_t gyro;
} mpu_motion_t;

bool mpu_read_motion_raw(mpu_motion_t *motion);

typedef struct {
  int32_t x_mg;
  int32_t y_mg;
  int32_t z_mg;
} mpu_accel_scaled_t;

typedef struct {
  int32_t x_mdps;
  int32_t y_mdps;
  int32_t z_mdps;
} mpu_gyro_scaled_t;

typedef struct {
  mpu_accel_scaled_t accel;
  int16_t temp_raw;
  mpu_gyro_scaled_t gyro;
} mpu_motion_scaled_t;

bool mpu_scale_motion(const mpu_motion_t *raw, mpu_motion_scaled_t *scaled);
bool mpu_read_motion_scaled(mpu_motion_scaled_t *motion);
