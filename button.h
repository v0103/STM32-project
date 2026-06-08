#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"

typedef enum {
  BUTTON_NONE,
  BUTTON_SHORT,
  BUTTON_LONG,
} button_event_t;

void button_init(uint32_t now);
button_event_t button_poll(uint32_t now);
void button_irq_init(void);
void button_irq_set_notify_task(TaskHandle_t task);
