#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
  BUTTON_NONE,
  BUTTON_SHORT,
  BUTTON_LONG,
} button_event_t;

void button_init(uint32_t now);
button_event_t button_poll(uint32_t now);
void button_irq_init(void);
bool button_take_recenter_request(uint32_t now);
