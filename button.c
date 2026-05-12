#include "button.h"
#include "hal.h"

static bool last_raw_pressed;   // 上一次看到的 raw pressed
static bool stable_pressed;     // debounce 後的穩定 pressed
static bool long_reported;      // 長按是否已經回報
static uint32_t changed_at;     // raw 狀態最後一次變化時間
static uint32_t pressed_at;     // 穩定按下的開始時間

enum {
  BUTTON_PIN = PIN('A', 4),
  BUTTON_DEBOUNCE_MS = 30,
  BUTTON_LONG_PRESS_MS = 1000,
};

void button_init(uint32_t now) {
    gpio_set_mode(BUTTON_PIN, GPIO_CFG_IN_PULL);
    gpio_write(BUTTON_PIN, true);   // pull-up
    bool raw_pressed = !gpio_read(BUTTON_PIN);

    last_raw_pressed = raw_pressed;
    stable_pressed = raw_pressed;
    long_reported = false;
    changed_at = now;
    pressed_at = now; 
}
button_event_t button_poll(uint32_t now) {
  // 1. read raw pressed
  bool raw_pressed = !gpio_read(BUTTON_PIN );  // active low
  // 2. detect raw change
  if(raw_pressed != last_raw_pressed){
    last_raw_pressed = raw_pressed;
    changed_at = now;
  }
  // 3. debounce guard
  if((now - changed_at) < BUTTON_DEBOUNCE_MS){
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
      if(!long_reported && press_duration < BUTTON_LONG_PRESS_MS){
        return BUTTON_SHORT;
      }
    }
  }  

  // 5. long press check
  if (stable_pressed && !long_reported && (now - pressed_at) >= BUTTON_LONG_PRESS_MS) {
    long_reported = true;
    return BUTTON_LONG;
  }
  // 6. default none
  return BUTTON_NONE;
}