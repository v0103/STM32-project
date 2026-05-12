#include "buzzer.h"
#include "hal.h"

enum {
  BUZZER_PIN = PIN('B', 12),
};

void buzzer_init(void) {
  gpio_set_mode(BUZZER_PIN, GPIO_CFG_OUT_PP_2MHZ);
  buzzer_off();
}

void buzzer_set(bool on) {
  gpio_write(BUZZER_PIN, !on);
}

void buzzer_on(void) {
  buzzer_set(true);
}

void buzzer_off(void) {
  buzzer_set(false);
}
