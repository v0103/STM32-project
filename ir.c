#include "ir.h"
#include "hal.h"

enum {
  IR_PIN = PIN('B', 13),
};

void ir_init(void) {
  gpio_set_mode(IR_PIN, GPIO_CFG_IN_FLOATING);
}

bool ir_motion_detected(void) {
  return !gpio_read(IR_PIN);  // active-low
}
