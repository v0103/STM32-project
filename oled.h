#pragma once

#include "gesture.h"

#include <stdbool.h>
#include <stdint.h>

bool oled_init(void);
bool oled_clear(void);
bool oled_write_text(uint8_t col, uint8_t page, const char *text);
bool oled_show_control(const gesture_control_t *control);
