#pragma once

#include "mpu.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct {
  int16_t roll_deg;
  int16_t pitch_deg;
} gesture_angles_t;

typedef enum {
  GESTURE_CENTER,
  GESTURE_LEFT,
  GESTURE_RIGHT,
  GESTURE_FORWARD,
  GESTURE_BACK,
  GESTURE_LEFT_FORWARD,
  GESTURE_RIGHT_FORWARD,
  GESTURE_LEFT_BACK,
  GESTURE_RIGHT_BACK,
} gesture_t;

typedef struct {
  gesture_angles_t angles;
  int8_t roll_axis;
  int8_t pitch_axis;
  gesture_t gesture;
} gesture_control_t;

void gesture_init(void);
void gesture_recenter(void);
bool gesture_update_angles(const mpu_motion_scaled_t *motion,
                           gesture_angles_t *angles);
gesture_t gesture_classify(const gesture_angles_t *angles);
gesture_t gesture_update_stable(gesture_t candidate);
bool gesture_update_control(const mpu_motion_scaled_t *motion,
                            gesture_control_t *control);
const char *gesture_name(gesture_t gesture);
