#include "hal.h"
#include "button.h"

static volatile uint32_t s_ticks; // volatile is important!!

void SysTick_Handler(void) {
  s_ticks++;
}


int main(void) {
  // uint16_t buzzer = PIN('B', 12);                // buzzer is on PB12
  uint16_t ir = PIN('B', 13);                    // IR receiver is on PB13
  uint16_t button = PIN('A', 4);                    // Button is on PA4

  systick_init(FREQ / 1000);                  // Tick every 1 ms
  uart_init(USART1, 9600);

  // gpio_set_mode(buzzer, GPIO_CFG_OUT_PP_2MHZ);
  gpio_set_mode(ir, GPIO_CFG_IN_FLOATING);

  gpio_set_mode(button, GPIO_CFG_IN_PULL);
  gpio_write(button, true);   // pull-up
  button_init(button, s_ticks);


  uint32_t timer = 0;
  uint32_t period = 500;
  for (;;) {
    button_event_t event = button_poll(button, s_ticks);
    if (event == BUTTON_SHORT) {
      printf("BUTTON SHORT\r\n");
    } else if (event == BUTTON_LONG) {
      printf("BUTTON LONG\r\n");
    }
    if (timer_expired(&timer, period, s_ticks)) {
      static bool on;

      // gpio_write(buzzer, on);
      on = !on;

      bool ir_state = gpio_read(ir);

      printf("BUZZER=%d IR=%d tick=%lu\r\n",
            on, ir_state, s_ticks);
    }
  }
  return 0;
}
