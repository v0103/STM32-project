#pragma once

#include <stdint.h>

typedef enum {
  BUTTON_NONE,
  BUTTON_SHORT,
  BUTTON_LONG,
} button_event_t;

void button_init(uint16_t button, uint32_t now);
button_event_t button_poll(uint16_t button, uint32_t now);
