#include "oled.h"
#include "i2c.h"

#include <stddef.h>
#include <stdio.h>

enum {
  OLED_ADDR = 0x3C,
  OLED_WIDTH = 128,
  OLED_PAGES = 8,
  OLED_TEXT_WIDTH = 6,
};

static bool oled_write_command(uint8_t command) {
  uint8_t data[] = {0x00, command};
  return i2c_write(OLED_ADDR, data, (uint16_t) sizeof(data));
}

static bool oled_write_data(const uint8_t *data, uint16_t len) {
  uint8_t packet[17];
  uint16_t sent = 0;

  while (sent < len) {
    uint16_t chunk = (uint16_t)(len - sent);
    uint16_t i;

    if (chunk > 16U) {
      chunk = 16;
    }

    packet[0] = 0x40;
    for (i = 0; i < chunk; i++) {
      packet[i + 1U] = data[sent + i];
    }

    if (!i2c_write(OLED_ADDR, packet, (uint16_t)(chunk + 1U))) {
      return false;
    }

    sent = (uint16_t)(sent + chunk);
  }

  return true;
}

static bool oled_set_cursor(uint8_t col, uint8_t page) {
  if (col >= OLED_WIDTH || page >= OLED_PAGES) {
    return false;
  }

  return oled_write_command((uint8_t)(0xB0U | page)) &&
         oled_write_command((uint8_t)(0x00U | (col & 0x0FU))) &&
         oled_write_command((uint8_t)(0x10U | (col >> 4)));
}

static void font5x7(char ch, uint8_t out[5]) {
  out[0] = 0;
  out[1] = 0;
  out[2] = 0;
  out[3] = 0;
  out[4] = 0;

  switch (ch) {
    case '0': out[0] = 0x3E; out[1] = 0x51; out[2] = 0x49; out[3] = 0x45; out[4] = 0x3E; break;
    case '1': out[0] = 0x00; out[1] = 0x42; out[2] = 0x7F; out[3] = 0x40; out[4] = 0x00; break;
    case '2': out[0] = 0x42; out[1] = 0x61; out[2] = 0x51; out[3] = 0x49; out[4] = 0x46; break;
    case '3': out[0] = 0x21; out[1] = 0x41; out[2] = 0x45; out[3] = 0x4B; out[4] = 0x31; break;
    case '4': out[0] = 0x18; out[1] = 0x14; out[2] = 0x12; out[3] = 0x7F; out[4] = 0x10; break;
    case '5': out[0] = 0x27; out[1] = 0x45; out[2] = 0x45; out[3] = 0x45; out[4] = 0x39; break;
    case '6': out[0] = 0x3C; out[1] = 0x4A; out[2] = 0x49; out[3] = 0x49; out[4] = 0x30; break;
    case '7': out[0] = 0x01; out[1] = 0x71; out[2] = 0x09; out[3] = 0x05; out[4] = 0x03; break;
    case '8': out[0] = 0x36; out[1] = 0x49; out[2] = 0x49; out[3] = 0x49; out[4] = 0x36; break;
    case '9': out[0] = 0x06; out[1] = 0x49; out[2] = 0x49; out[3] = 0x29; out[4] = 0x1E; break;
    case 'A': out[0] = 0x7E; out[1] = 0x11; out[2] = 0x11; out[3] = 0x11; out[4] = 0x7E; break;
    case 'B': out[0] = 0x7F; out[1] = 0x49; out[2] = 0x49; out[3] = 0x49; out[4] = 0x36; break;
    case 'C': out[0] = 0x3E; out[1] = 0x41; out[2] = 0x41; out[3] = 0x41; out[4] = 0x22; break;
    case 'D': out[0] = 0x7F; out[1] = 0x41; out[2] = 0x41; out[3] = 0x22; out[4] = 0x1C; break;
    case 'E': out[0] = 0x7F; out[1] = 0x49; out[2] = 0x49; out[3] = 0x49; out[4] = 0x41; break;
    case 'F': out[0] = 0x7F; out[1] = 0x09; out[2] = 0x09; out[3] = 0x09; out[4] = 0x01; break;
    case 'G': out[0] = 0x3E; out[1] = 0x41; out[2] = 0x49; out[3] = 0x49; out[4] = 0x7A; break;
    case 'H': out[0] = 0x7F; out[1] = 0x08; out[2] = 0x08; out[3] = 0x08; out[4] = 0x7F; break;
    case 'I': out[0] = 0x00; out[1] = 0x41; out[2] = 0x7F; out[3] = 0x41; out[4] = 0x00; break;
    case 'K': out[0] = 0x7F; out[1] = 0x08; out[2] = 0x14; out[3] = 0x22; out[4] = 0x41; break;
    case 'L': out[0] = 0x7F; out[1] = 0x40; out[2] = 0x40; out[3] = 0x40; out[4] = 0x40; break;
    case 'M': out[0] = 0x7F; out[1] = 0x02; out[2] = 0x0C; out[3] = 0x02; out[4] = 0x7F; break;
    case 'N': out[0] = 0x7F; out[1] = 0x04; out[2] = 0x08; out[3] = 0x10; out[4] = 0x7F; break;
    case 'O': out[0] = 0x3E; out[1] = 0x41; out[2] = 0x41; out[3] = 0x41; out[4] = 0x3E; break;
    case 'P': out[0] = 0x7F; out[1] = 0x09; out[2] = 0x09; out[3] = 0x09; out[4] = 0x06; break;
    case 'R': out[0] = 0x7F; out[1] = 0x09; out[2] = 0x19; out[3] = 0x29; out[4] = 0x46; break;
    case 'S': out[0] = 0x46; out[1] = 0x49; out[2] = 0x49; out[3] = 0x49; out[4] = 0x31; break;
    case 'T': out[0] = 0x01; out[1] = 0x01; out[2] = 0x7F; out[3] = 0x01; out[4] = 0x01; break;
    case 'U': out[0] = 0x3F; out[1] = 0x40; out[2] = 0x40; out[3] = 0x40; out[4] = 0x3F; break;
    case 'W': out[0] = 0x7F; out[1] = 0x20; out[2] = 0x18; out[3] = 0x20; out[4] = 0x7F; break;
    case 'Y': out[0] = 0x07; out[1] = 0x08; out[2] = 0x70; out[3] = 0x08; out[4] = 0x07; break;
    case ' ': break;
    case '-': out[0] = 0x08; out[1] = 0x08; out[2] = 0x08; out[3] = 0x08; out[4] = 0x08; break;
    case ':': out[0] = 0x00; out[1] = 0x36; out[2] = 0x36; out[3] = 0x00; out[4] = 0x00; break;
    case '=': out[0] = 0x14; out[1] = 0x14; out[2] = 0x14; out[3] = 0x14; out[4] = 0x14; break;
    case '_': out[0] = 0x40; out[1] = 0x40; out[2] = 0x40; out[3] = 0x40; out[4] = 0x40; break;
    default: out[0] = 0x7F; out[1] = 0x41; out[2] = 0x5D; out[3] = 0x41; out[4] = 0x7F; break;
  }
}

bool oled_init(void) {
  static const uint8_t init_commands[] = {
    0xAE, 0x20, 0x00, 0xB0, 0xC8, 0x00, 0x10, 0x40,
    0x81, 0x7F, 0xA1, 0xA6, 0xA8, 0x3F, 0xA4, 0xD3,
    0x00, 0xD5, 0x80, 0xD9, 0xF1, 0xDA, 0x12, 0xDB,
    0x40, 0x8D, 0x14, 0xAF,
  };
  uint16_t i;

  if (!i2c_probe(OLED_ADDR)) {
    return false;
  }

  for (i = 0; i < (uint16_t) sizeof(init_commands); i++) {
    if (!oled_write_command(init_commands[i])) {
      return false;
    }
  }

  return oled_clear();
}

bool oled_clear(void) {
  uint8_t zeros[OLED_WIDTH];
  uint8_t page;
  uint16_t i;

  for (i = 0; i < OLED_WIDTH; i++) {
    zeros[i] = 0;
  }

  for (page = 0; page < OLED_PAGES; page++) {
    if (!oled_set_cursor(0, page) ||
        !oled_write_data(zeros, (uint16_t) sizeof(zeros))) {
      return false;
    }
  }

  return true;
}

bool oled_write_text(uint8_t col, uint8_t page, const char *text) {
  uint8_t glyph[6];

  if (text == NULL || !oled_set_cursor(col, page)) {
    return false;
  }

  while (*text != '\0' && col <= (OLED_WIDTH - OLED_TEXT_WIDTH)) {
    font5x7(*text, glyph);
    glyph[5] = 0;

    if (!oled_write_data(glyph, (uint16_t) sizeof(glyph))) {
      return false;
    }

    col = (uint8_t)(col + OLED_TEXT_WIDTH);
    text++;
  }

  return true;
}

bool oled_show_control(const gesture_control_t *control) {
  char line[22];

  if (control == NULL || !oled_clear()) {
    return false;
  }

  if (!oled_write_text(0, 0, "GESTURE")) {
    return false;
  }

  if (!oled_write_text(0, 1, gesture_name(control->gesture))) {
    return false;
  }

  (void) snprintf(line, sizeof(line), "ROLL=%d", control->angles.roll_deg);
  if (!oled_write_text(0, 3, line)) {
    return false;
  }

  (void) snprintf(line, sizeof(line), "PITCH=%d", control->angles.pitch_deg);
  if (!oled_write_text(0, 4, line)) {
    return false;
  }

  (void) snprintf(line, sizeof(line), "RA=%d PA=%d",
                  control->roll_axis,
                  control->pitch_axis);
  return oled_write_text(0, 6, line);
}
