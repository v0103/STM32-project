#include "gesture.h"

#include <math.h>
#include <stddef.h>

enum {
  GESTURE_EMA_DIVISOR = 4,
  GESTURE_DEADZONE_DEG = 5,
  GESTURE_MAX_ANGLE_DEG = 45,
  GESTURE_AXIS_THRESHOLD = 1,
  GESTURE_CONFIRM_COUNT = 3,
};

static bool s_filter_ready;
static int32_t s_accel_x_mg;
static int32_t s_accel_y_mg;
static int32_t s_accel_z_mg;
static int32_t s_center_roll_deg;
static int32_t s_center_pitch_deg;
static int16_t s_last_raw_roll_deg;
static int16_t s_last_raw_pitch_deg;
static gesture_t s_stable_gesture;
static gesture_t s_candidate_gesture;
static uint8_t s_candidate_count;

static int32_t ema_update(int32_t filtered, int32_t sample) {
  return filtered + ((sample - filtered) / GESTURE_EMA_DIVISOR);
}

static int16_t float_to_i16_deg(float value) {
  if (value > (float) INT16_MAX) {
    return INT16_MAX;
  }

  if (value < (float) INT16_MIN) {
    return INT16_MIN;
  }

  if (value >= 0.0f) {
    return (int16_t)(value + 0.5f);
  }

  return (int16_t)(value - 0.5f);
}

static int16_t i16_abs(int16_t value) {
  if (value < 0) {
    return (int16_t)-value;
  }

  return value;
}

static int8_t angle_to_axis(int16_t angle_deg) {
  int16_t magnitude = i16_abs(angle_deg);
  int32_t axis;

  if (magnitude <= GESTURE_DEADZONE_DEG) {
    return 0;
  }

  if (magnitude >= GESTURE_MAX_ANGLE_DEG) {
    return angle_deg < 0 ? (int8_t)-100 : (int8_t)100;
  }

  axis = ((int32_t)(magnitude - GESTURE_DEADZONE_DEG) * 100) /
         (GESTURE_MAX_ANGLE_DEG - GESTURE_DEADZONE_DEG);

  if (angle_deg < 0) {
    axis = -axis;
  }

  return (int8_t)axis;
}

static gesture_t gesture_from_axes(int8_t roll_axis, int8_t pitch_axis) {
  bool left = roll_axis < -GESTURE_AXIS_THRESHOLD;
  bool right = roll_axis > GESTURE_AXIS_THRESHOLD;
  bool forward = pitch_axis > GESTURE_AXIS_THRESHOLD;
  bool back = pitch_axis < -GESTURE_AXIS_THRESHOLD;

  if (left && forward) {
    return GESTURE_LEFT_FORWARD;
  }

  if (right && forward) {
    return GESTURE_RIGHT_FORWARD;
  }

  if (left && back) {
    return GESTURE_LEFT_BACK;
  }

  if (right && back) {
    return GESTURE_RIGHT_BACK;
  }

  if (left) {
    return GESTURE_LEFT;
  }

  if (right) {
    return GESTURE_RIGHT;
  }

  if (forward) {
    return GESTURE_FORWARD;
  }

  if (back) {
    return GESTURE_BACK;
  }

  return GESTURE_CENTER;
}

void gesture_init(void) {
  s_filter_ready = false;
  s_accel_x_mg = 0;
  s_accel_y_mg = 0;
  s_accel_z_mg = 0;
  s_center_roll_deg = 0;
  s_center_pitch_deg = 0;
  s_last_raw_roll_deg = 0;
  s_last_raw_pitch_deg = 0;
  s_stable_gesture = GESTURE_CENTER;
  s_candidate_gesture = GESTURE_CENTER;
  s_candidate_count = 0;
}

void gesture_recenter(void) {
  if (!s_filter_ready) {
    return;
  }

  s_center_roll_deg = s_last_raw_roll_deg;
  s_center_pitch_deg = s_last_raw_pitch_deg;
  s_stable_gesture = GESTURE_CENTER;
  s_candidate_gesture = GESTURE_CENTER;
  s_candidate_count = 0;
}

bool gesture_update_angles(const mpu_motion_scaled_t *motion,
                           gesture_angles_t *angles) {
  float ax;
  float ay;
  float az;
  float roll;
  float pitch;
  float pitch_denominator;

  if (motion == NULL || angles == NULL) {
    return false;
  }

  if (!s_filter_ready) {
    s_accel_x_mg = motion->accel.x_mg;
    s_accel_y_mg = motion->accel.y_mg;
    s_accel_z_mg = motion->accel.z_mg;
    s_filter_ready = true;
  } else {
    s_accel_x_mg = ema_update(s_accel_x_mg, motion->accel.x_mg);
    s_accel_y_mg = ema_update(s_accel_y_mg, motion->accel.y_mg);
    s_accel_z_mg = ema_update(s_accel_z_mg, motion->accel.z_mg);
  }

  ax = (float) s_accel_x_mg;
  ay = (float) s_accel_y_mg;
  az = (float) s_accel_z_mg;

  roll = atan2f(ay, az) * (180.0f / 3.1415926f);
  pitch_denominator = sqrtf((ay * ay) + (az * az));
  pitch = atan2f(-ax, pitch_denominator) * (180.0f / 3.1415926f);

  s_last_raw_roll_deg = float_to_i16_deg(roll);
  s_last_raw_pitch_deg = float_to_i16_deg(pitch);

  angles->roll_deg = (int16_t)(s_last_raw_roll_deg - s_center_roll_deg);
  angles->pitch_deg = (int16_t)(s_last_raw_pitch_deg - s_center_pitch_deg);

  return true;
}

gesture_t gesture_classify(const gesture_angles_t *angles) {
  int8_t roll_axis;
  int8_t pitch_axis;

  if (angles == NULL) {
    return GESTURE_CENTER;
  }

  roll_axis = angle_to_axis(angles->roll_deg);
  pitch_axis = angle_to_axis(angles->pitch_deg);

  return gesture_from_axes(roll_axis, pitch_axis);
}

gesture_t gesture_update_stable(gesture_t candidate) {
  if (candidate != s_candidate_gesture) {
    s_candidate_gesture = candidate;
    s_candidate_count = 1;
  } else if (s_candidate_count < GESTURE_CONFIRM_COUNT) {
    s_candidate_count++;
  }

  if (s_candidate_count >= GESTURE_CONFIRM_COUNT) {
    s_stable_gesture = s_candidate_gesture;
  }

  return s_stable_gesture;
}

bool gesture_update_control(const mpu_motion_scaled_t *motion,
                            gesture_control_t *control) {
  gesture_t candidate;

  if (motion == NULL || control == NULL) {
    return false;
  }

  if (!gesture_update_angles(motion, &control->angles)) {
    return false;
  }

  control->roll_axis = angle_to_axis(control->angles.roll_deg);
  control->pitch_axis = angle_to_axis(control->angles.pitch_deg);
  candidate = gesture_from_axes(control->roll_axis, control->pitch_axis);
  control->gesture = gesture_update_stable(candidate);

  return true;
}

const char *gesture_name(gesture_t gesture) {
  switch (gesture) {
    case GESTURE_CENTER:
      return "CENTER";
    case GESTURE_LEFT:
      return "LEFT";
    case GESTURE_RIGHT:
      return "RIGHT";
    case GESTURE_FORWARD:
      return "FORWARD";
    case GESTURE_BACK:
      return "BACK";
    case GESTURE_LEFT_FORWARD:
      return "LEFT_FORWARD";
    case GESTURE_RIGHT_FORWARD:
      return "RIGHT_FORWARD";
    case GESTURE_LEFT_BACK:
      return "LEFT_BACK";
    case GESTURE_RIGHT_BACK:
      return "RIGHT_BACK";
    default:
      return "?";
  }
}
