#include "hal.h"
#include "button.h"
#include "sensor.h"
#include "buzzer.h"
#include "ir.h"
#include "i2c.h"
#include "mpu.h"

static volatile uint32_t s_ticks; // volatile is important!!

void SysTick_Handler(void) {
  s_ticks++;
}

int main(void) {
  systick_init(FREQ / 1000);                  // Tick every 1 ms
  uart_init(USART1, 9600);

  // ir_init();
  // buzzer_init();
  // button_init(s_ticks);
  // sensor_init();
  soft_i2c_init();
  bool mpu_ok = mpu_init();
  printf("MPU init=%d\r\n", mpu_ok);
  bool gyro_cal_ok = mpu_calibrate_gyro(64);
  printf("GYRO cal=%d\r\n", gyro_cal_ok);
  uint8_t pwr = 0;
  mpu_read_reg(0x6B, &pwr);
  printf("PWR_MGMT_1 after init=0x%02X\r\n", pwr);
  // bool alarm_condition = false;
  // bool buzzer_muted = false;
  // uint32_t last_motion_at = 0;

  uint32_t timer = s_ticks;
  uint32_t period = 500;
  for (;;) {
    uint32_t now = s_ticks;

    // bool motion = ir_motion_detected();
    // if (motion) {
    //   last_motion_at = now;

    //   if (!alarm_condition) {
    //     alarm_condition = true;
    //     buzzer_muted = false;
    //   }
    // }
    // if (alarm_condition && !motion && (now - last_motion_at) >= 5000) {
    //   alarm_condition = false;
    //   buzzer_muted = false;
    //   printf("ALARM CLEARED\r\n");
    // }

    // button_event_t event = button_poll(now);
    // if (event == BUTTON_SHORT) {
    //   printf("BUTTON SHORT\r\n");
    // } else if (event == BUTTON_LONG) {
    //   if (alarm_condition) {
    //     buzzer_muted = true;
    //     printf("ALARM ACK: buzzer muted\r\n");
    //   }
    // }
    
    // buzzer_set(alarm_condition && !buzzer_muted);

    if (timer_expired(&timer, period, now)) {
      // sensor_data_t sensor = sensor_read();

      // printf("MPU 0x68=%d 0x69=%d\r\n",
      //  i2c_probe(0x68),
      //  i2c_probe(0x69));

      mpu_motion_t motion;
      mpu_motion_scaled_t scaled;
      if (mpu_read_motion_raw(&motion)) {
        (void) mpu_scale_motion(&motion, &scaled);
        printf("ACCEL x=%d y=%d z=%d TEMP=%d\r\n",
               motion.accel.x, motion.accel.y, motion.accel.z,
               motion.temp_raw);
        printf("GYRO x=%d y=%d z=%d\r\n",
               motion.gyro.x, motion.gyro.y, motion.gyro.z);
        printf("ACCEL mg x=%ld y=%ld z=%ld\r\n",
               scaled.accel.x_mg, scaled.accel.y_mg, scaled.accel.z_mg);
        printf("GYRO mdps x=%ld y=%ld z=%ld\r\n",
               scaled.gyro.x_mdps, scaled.gyro.y_mdps, scaled.gyro.z_mdps);
      } else {
        printf("MPU motion read failed\r\n");
      }

      // printf("MOTION=%d ALARM=%d MUTED=%d THERM_RAW=%u LIGHT_RAW=%u HEAT=%u LIGHT=%u tick=%lu\r\n\n",
      //       motion,
      //       alarm_condition,
      //       buzzer_muted,
      //       sensor.therm_raw,
      //       sensor.light_raw,
      //       sensor.heat_level,
      //       sensor.light_level,
      //       now);

    }
  }
  return 0;
}
