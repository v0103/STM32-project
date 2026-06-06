#include "app.h"

#include "button.h"
#include "control_packet.h"
#include "gesture.h"
#include "hal.h"
#include "i2c.h"
#include "mpu.h"
#include "oled.h"

#include <stdio.h>

enum {
  APP_CONTROL_PERIOD_MS = 100,
  APP_OLED_PERIOD_MS = 500,
};

static uint32_t s_control_timer;
static uint32_t s_oled_timer;
static bool s_oled_ready;

bool app_init(uint32_t now) {
  bool mpu_ok;
  bool gyro_cal_ok;
  uint8_t pwr = 0;

  button_irq_init();
  soft_i2c_init();

  s_oled_ready = oled_init();
  printf("OLED init=%d\r\n", s_oled_ready);

  mpu_ok = mpu_init();
  printf("MPU init=%d\r\n", mpu_ok);
  if (!mpu_ok) {
    return false;
  }

  gyro_cal_ok = mpu_calibrate_gyro(64);
  printf("GYRO cal=%d\r\n", gyro_cal_ok);

  gesture_init();

  if (mpu_read_reg(0x6B, &pwr)) {
    printf("PWR_MGMT_1 after init=0x%02X\r\n", pwr);
  }

  s_control_timer = now;
  s_oled_timer = now;
  return gyro_cal_ok;
}

void app_update(uint32_t now) {
  mpu_motion_t motion;
  mpu_motion_scaled_t scaled;
  gesture_control_t control;

  if (!timer_expired(&s_control_timer, APP_CONTROL_PERIOD_MS, now)) {
    return;
  }

  if (!mpu_read_motion_raw(&motion)) {
    printf("MPU motion read failed\r\n");
    return;
  }

  if (!mpu_scale_motion(&motion, &scaled) ||
      !gesture_update_control(&scaled, &control)) {
    printf("Control update failed\r\n");
    return;
  }

  if (button_take_recenter_request(now)) {
    gesture_recenter();
    (void) gesture_update_control(&scaled, &control);

    if (s_oled_ready) {
      (void) oled_show_recenter();
      s_oled_timer = now + APP_OLED_PERIOD_MS;
    }

    printf("RECENTER\r\n");
  }

  control_packet_print(now, &control);

  if (s_oled_ready &&
      timer_expired(&s_oled_timer, APP_OLED_PERIOD_MS, now)) {
    (void) oled_show_control(&control);
  }
}
