#pragma once

#include <stdbool.h>
#include <stdint.h>

bool app_init(void);
bool app_start_tasks(void);
void app_update(uint32_t now);
