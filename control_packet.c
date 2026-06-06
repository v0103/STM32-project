#include "control_packet.h"

#include <stdio.h>

bool control_packet_format(char *buf,
                           size_t len,
                           uint32_t tick_ms,
                           const gesture_control_t *control) {
  int written;

  if (buf == NULL || len == 0 || control == NULL) {
    return false;
  }

  written = snprintf(buf,
                     len,
                     "CTRL,%lu,%d,%d,%d,%d,%s\r\n",
                     tick_ms,
                     control->angles.roll_deg,
                     control->angles.pitch_deg,
                     control->roll_axis,
                     control->pitch_axis,
                     gesture_name(control->gesture));

  if (written < 0 || (size_t) written >= len) {
    buf[0] = '\0';
    return false;
  }

  return true;
}

void control_packet_print(uint32_t tick_ms, const gesture_control_t *control) {
  char packet[64];

  if (control_packet_format(packet, sizeof(packet), tick_ms, control)) {
    printf("%s", packet);
  }
}
