#include "hal.h"
#include "button.h"
#include "sensor.h"
#include "buzzer.h"
#include "ir.h"

static volatile uint32_t s_ticks; // volatile is important!!

void SysTick_Handler(void) {
  s_ticks++;
}

int main(void) {
  systick_init(FREQ / 1000);                  // Tick every 1 ms
  uart_init(USART1, 9600);

  ir_init();
  buzzer_init();
  button_init(s_ticks);
  sensor_init();

  bool alarm_condition = false;
  bool buzzer_muted = false;
  uint32_t last_motion_at = 0;

  uint32_t timer = 0;
  uint32_t period = 500;
  for (;;) {
    uint32_t now = s_ticks;

    bool motion = ir_motion_detected();
    if (motion) {
      last_motion_at = now;

      if (!alarm_condition) {
        alarm_condition = true;
        buzzer_muted = false;
      }
    }
    if (alarm_condition && !motion && (now - last_motion_at) >= 5000) {
      alarm_condition = false;
      buzzer_muted = false;
      printf("ALARM CLEARED\r\n");
    }

    button_event_t event = button_poll(now);
    if (event == BUTTON_SHORT) {
      printf("BUTTON SHORT\r\n");
    } else if (event == BUTTON_LONG) {
      if (alarm_condition) {
        buzzer_muted = true;
        printf("ALARM ACK: buzzer muted\r\n");
      }
    }
    
    buzzer_set(alarm_condition && !buzzer_muted);

    if (timer_expired(&timer, period, now)) {
      sensor_data_t sensor = sensor_read();

      printf("MOTION=%d ALARM=%d MUTED=%d THERM_RAW=%u LIGHT_RAW=%u HEAT=%u LIGHT=%u tick=%lu\r\n",
            motion,
            alarm_condition,
            buzzer_muted,
            sensor.therm_raw,
            sensor.light_raw,
            sensor.heat_level,
            sensor.light_level,
            now);

    }
  }
  return 0;
}
