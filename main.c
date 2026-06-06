#include "hal.h"
#include "app.h"

static volatile uint32_t s_ticks; // volatile is important!!

void SysTick_Handler(void) {
  s_ticks++;
}

int main(void) {
  systick_init(FREQ / 1000);  // Tick every 1 ms
  uart_init(USART1, 115200);

  bool app_ready = app_init(s_ticks);
  if (!app_ready) {
    printf("APP init failed\r\n");
  }

  for (;;) {
    if (app_ready) {
      app_update(s_ticks);
    }
  }

  return 0;
}
