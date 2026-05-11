#include "button.h"
#include "hal.h"

static bool last_raw_pressed;   // 上一次看到的 raw pressed
static bool stable_pressed;     // debounce 後的穩定 pressed
static bool long_reported;      // 長按是否已經回報
static uint32_t changed_at;     // raw 狀態最後一次變化時間
static uint32_t pressed_at;     // 穩定按下的開始時間

void button_init(uint16_t button, uint32_t now) {
    bool raw_pressed = !gpio_read(button);

    last_raw_pressed = raw_pressed;
    stable_pressed = raw_pressed;
    long_reported = false;
    changed_at = now;
    pressed_at = now; 
}
button_event_t button_poll(uint16_t button, uint32_t now) {
  enum {
    DEBOUNCE_MS = 30,
    LONG_PRESS_MS = 1000,
  };

  // 1. read raw pressed
  bool raw_pressed = !gpio_read(button);  // active low
  // 2. detect raw change
  if(raw_pressed != last_raw_pressed){
    last_raw_pressed = raw_pressed;
    changed_at = now;
  }
  // 3. debounce guard
  if((now - changed_at) < DEBOUNCE_MS){
    return BUTTON_NONE;
  }
  // 4. stable state change
  if(raw_pressed != stable_pressed){
    stable_pressed = raw_pressed;
    if (stable_pressed) {
      pressed_at = now;
      long_reported = false;
    } else {
      uint32_t press_duration = now - pressed_at;
      if(press_duration < 1000){
        return BUTTON_SHORT;
      }
    }
  }  

  // 5. long press check
  if (stable_pressed && !long_reported && (now - pressed_at) >= 1000) {
    long_reported = true;
    return BUTTON_LONG;
  }
  // 6. default none
  return BUTTON_NONE;
}