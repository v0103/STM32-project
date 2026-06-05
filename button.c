#include "button.h"
#include "hal.h"

static bool last_raw_pressed;
static bool stable_pressed;
static bool long_reported;
static uint32_t changed_at;
static uint32_t pressed_at;
static volatile bool irq_recenter_requested;
static uint32_t last_recenter_at;

enum {
  BUTTON_PIN = PIN('A', 4),
  BUTTON_DEBOUNCE_MS = 30,
  BUTTON_LONG_PRESS_MS = 1000,
  BUTTON_RECENTER_DEBOUNCE_MS = 200,
};

static void button_gpio_init(void) {
  gpio_set_mode(BUTTON_PIN, GPIO_CFG_IN_PULL);
  gpio_write(BUTTON_PIN, true);  // pull-up: released = 1, pressed = 0
}

void button_init(uint32_t now) {
  bool raw_pressed;

  button_gpio_init();
  raw_pressed = !gpio_read(BUTTON_PIN);

  last_raw_pressed = raw_pressed;
  stable_pressed = raw_pressed;
  long_reported = false;
  changed_at = now;
  pressed_at = now;
}

button_event_t button_poll(uint32_t now) {
  bool raw_pressed = !gpio_read(BUTTON_PIN);  // active low

  if (raw_pressed != last_raw_pressed) {
    last_raw_pressed = raw_pressed;
    changed_at = now;
  }

  if ((now - changed_at) < BUTTON_DEBOUNCE_MS) {
    return BUTTON_NONE;
  }

  if (raw_pressed != stable_pressed) {
    stable_pressed = raw_pressed;
    if (stable_pressed) {
      pressed_at = now;
      long_reported = false;
    } else {
      uint32_t press_duration = now - pressed_at;
      if (!long_reported && press_duration < BUTTON_LONG_PRESS_MS) {
        return BUTTON_SHORT;
      }
    }
  }

  if (stable_pressed && !long_reported &&
      (now - pressed_at) >= BUTTON_LONG_PRESS_MS) {
    long_reported = true;
    return BUTTON_LONG;
  }

  return BUTTON_NONE;
}

void button_irq_init(void) {
  button_gpio_init();
  irq_recenter_requested = false;
  last_recenter_at = 0;

  RCC->APB2ENR |= BIT(0);           // AFIO clock
  AFIO->EXTICR[1] &= ~(0xFU << 0);  // EXTI4 source = PA4

  EXTI->IMR |= BIT(4);    // unmask EXTI line 4
  EXTI->RTSR &= ~BIT(4);  // no rising-edge interrupt
  EXTI->FTSR |= BIT(4);   // falling edge: button pressed
  EXTI->PR = BIT(4);      // clear stale pending flag

  NVIC_EnableIRQ(EXTI4_IRQn);
}

bool button_take_recenter_request(uint32_t now) {
  bool requested = irq_recenter_requested;

  if (!requested) {
    return false;
  }

  irq_recenter_requested = false;

  if (last_recenter_at != 0 &&
      (now - last_recenter_at) < BUTTON_RECENTER_DEBOUNCE_MS) {
    return false;
  }

  last_recenter_at = now;
  return true;
}

void EXTI4_IRQHandler(void) {
  if ((EXTI->PR & BIT(4)) != 0) {
    EXTI->PR = BIT(4);
    irq_recenter_requested = true;
  }
}
