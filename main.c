#include "hal.h"

static volatile uint32_t s_ticks; // volatile is important!!

void SysTick_Handler(void) {
  s_ticks++;
}

int main(void) {
  uint16_t buzzer = PIN('B', 12);                // buzzer is on PB12
  uint16_t ir = PIN('B', 13);                    // IR receiver is on PB13
  uint16_t button = PIN('A', 4);                    // Button is on PA4

  systick_init(FREQ / 1000);                  // Tick every 1 ms
  uart_init(USART1, 9600);

  gpio_set_mode(buzzer, GPIO_CFG_OUT_PP_2MHZ);
  gpio_set_mode(ir, GPIO_CFG_IN_FLOATING);
  gpio_set_mode(button, GPIO_CFG_IN_PULL);
  gpio_write(button, true);   // pull-up

  uint32_t timer = 0, period = 500;
  for (;;) {
    if (timer_expired(&timer, period, s_ticks)) {
      static bool buzzer_on=true;
      gpio_write(buzzer, buzzer_on);
      // buzzer_on = !buzzer_on;

      bool ir_state = !gpio_read(ir);
      bool button_state = !gpio_read(button);
      printf("BUZZER=%d, IR=%d, Button=%d\r\n", buzzer_on, ir_state, button_state);

    }
  }
  return 0;
}
