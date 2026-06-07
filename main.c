#include "hal.h"
#include "app.h"

#include "FreeRTOS.h"
#include "task.h"

int main(void) {
  uart_init(USART1, 115200);

  if (!app_init()) {
    printf("APP init failed\r\n");
    for (;;) {
    }
  }

  if (!app_start_tasks()) {
    printf("APP task create failed\r\n");
    for (;;) {
    }
  }

  vTaskStartScheduler();

  printf("Scheduler start failed\r\n");
  for (;;) {
  }
}
