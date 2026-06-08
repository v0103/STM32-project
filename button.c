#include "button.h"
#include "hal.h"

static bool last_raw_pressed;
static bool stable_pressed;
static bool long_reported;
static uint32_t changed_at;
static uint32_t pressed_at;
static TaskHandle_t irq_notify_task;

enum {
  BUTTON_PIN = PIN('A', 1),
  BUTTON_DEBOUNCE_MS = 30,
  BUTTON_LONG_PRESS_MS = 1000,
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
  irq_notify_task = NULL;

  RCC->APB2ENR |= BIT(0);            // AFIO clock
  AFIO->EXTICR[0] &= ~(0xFU << 4);   // EXTI1 source = PA1

  EXTI->IMR |= BIT(1);     // unmask EXTI line 1
  EXTI->RTSR &= ~BIT(1);   // no rising-edge interrupt
  EXTI->FTSR |= BIT(1);    // falling edge: button pressed
  EXTI->PR = BIT(1);       // clear stale pending flag

  NVIC_SetPriority(EXTI1_IRQn, 5U);
  NVIC_EnableIRQ(EXTI1_IRQn);
}

void button_irq_set_notify_task(TaskHandle_t task) {
  irq_notify_task = task;
}

void EXTI1_IRQHandler(void) {
  if ((EXTI->PR & BIT(1)) != 0) {
    BaseType_t higher_priority_task_woken = pdFALSE;

    EXTI->PR = BIT(1);

    if (irq_notify_task != NULL) {
      vTaskNotifyGiveFromISR(irq_notify_task, &higher_priority_task_woken);
      portYIELD_FROM_ISR(higher_priority_task_woken);
    }
  }
}
