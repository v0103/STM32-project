#pragma once

#include "gesture.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool control_packet_format(char *buf,
                           size_t len,
                           uint32_t tick_ms,
                           const gesture_control_t *control);
void control_packet_print(uint32_t tick_ms, const gesture_control_t *control);
