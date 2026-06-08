#include "app.h"

#include "button.h"
#include "control_packet.h"
#include "gesture.h"
#include "hal.h"
#include "i2c.h"
#include "mpu.h"
#include "oled.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"

#include <stdio.h>

enum {
  APP_CONTROL_PERIOD_MS = 50,
  APP_OLED_PERIOD_MS = 500,
  APP_UART_PERIOD_MS = 100,
  APP_STACK_REPORT_PERIOD_MS = 5000,
  APP_RECENTER_DEBOUNCE_MS = 500,
  APP_CONTROL_TASK_STACK_WORDS = 512,
  APP_OLED_TASK_STACK_WORDS = 384,
  APP_UART_TASK_STACK_WORDS = 384,
  APP_CONTROL_TASK_PRIORITY = tskIDLE_PRIORITY + 2,
  APP_UART_TASK_PRIORITY = tskIDLE_PRIORITY + 1,
  APP_OLED_TASK_PRIORITY = tskIDLE_PRIORITY + 1,
};

typedef struct {
  uint32_t tick_ms;
  gesture_control_t control;
} app_control_sample_t;

static bool s_oled_ready;
static QueueHandle_t s_control_queue;
static SemaphoreHandle_t s_i2c_mutex;
static TaskHandle_t s_control_task;
static TaskHandle_t s_oled_task;
static TaskHandle_t s_uart_task;
static TickType_t s_last_recenter_tick;
static bool s_recenter_seen;

static bool app_i2c_take(void) {
  return s_i2c_mutex != NULL &&
         xSemaphoreTake(s_i2c_mutex, portMAX_DELAY) == pdTRUE;
}

static void app_i2c_give(void) {
  if (s_i2c_mutex != NULL) {
    (void) xSemaphoreGive(s_i2c_mutex);
  }
}

static void app_print_stack_report(void) {
  UBaseType_t control_words = 0;
  UBaseType_t oled_words = 0;
  UBaseType_t uart_words = 0;

  if (s_control_task != NULL) {
    control_words = uxTaskGetStackHighWaterMark(s_control_task);
  }

  if (s_oled_task != NULL) {
    oled_words = uxTaskGetStackHighWaterMark(s_oled_task);
  }

  if (s_uart_task != NULL) {
    uart_words = uxTaskGetStackHighWaterMark(s_uart_task);
  }

  printf("STACK,CONTROL=%lu,OLED=%lu,UART=%lu\r\n",
         (unsigned long) control_words,
         (unsigned long) oled_words,
         (unsigned long) uart_words);
}

static void app_control_task(void *argument) {
  TickType_t last_wake;

  (void) argument;
  last_wake = xTaskGetTickCount();

  for (;;) {
    vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(APP_CONTROL_PERIOD_MS));
    app_update((uint32_t) xTaskGetTickCount());
  }
}

static void app_oled_task(void *argument) {
  uint32_t notification_count;
  app_control_sample_t sample;

  (void) argument;

  for (;;) {
    notification_count = ulTaskNotifyTake(pdTRUE,
                                          pdMS_TO_TICKS(APP_OLED_PERIOD_MS));

    if (notification_count > 0U) {
      if (s_oled_ready && app_i2c_take()) {
        (void) oled_show_recenter();
        app_i2c_give();
      }
      continue;
    }

    if (s_oled_ready &&
        xQueuePeek(s_control_queue, &sample, 0) == pdPASS &&
        app_i2c_take()) {
      (void) oled_show_control(&sample.control);
      app_i2c_give();
    }
  }
}

static void app_uart_task(void *argument) {
  TickType_t last_wake;
  TickType_t last_stack_report;
  app_control_sample_t sample;

  (void) argument;
  last_wake = xTaskGetTickCount();
  last_stack_report = last_wake;

  for (;;) {
    TickType_t now;

    vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(APP_UART_PERIOD_MS));
    now = xTaskGetTickCount();

    if (xQueuePeek(s_control_queue, &sample, 0) == pdPASS) {
      control_packet_print(sample.tick_ms, &sample.control);
    }

    if ((now - last_stack_report) >=
        pdMS_TO_TICKS(APP_STACK_REPORT_PERIOD_MS)) {
      last_stack_report = now;
      app_print_stack_report();
    }
  }
}

bool app_init(void) {
  bool mpu_ok;
  bool gyro_cal_ok;
  uint8_t pwr = 0;

  s_control_queue = xQueueCreate(1, sizeof(app_control_sample_t));
  s_i2c_mutex = xSemaphoreCreateMutex();
  s_control_task = NULL;
  s_oled_task = NULL;
  s_uart_task = NULL;
  s_last_recenter_tick = 0;
  s_recenter_seen = false;
  if (s_control_queue == NULL || s_i2c_mutex == NULL) {
    printf("RTOS object create failed\r\n");
    return false;
  }

  button_irq_init();
  soft_i2c_init();

  s_oled_ready = oled_init();
  printf("OLED init=%d\r\n", s_oled_ready);

  mpu_ok = mpu_init();
  printf("MPU init=%d\r\n", mpu_ok);
  if (!mpu_ok) {
    return false;
  }

  gyro_cal_ok = mpu_calibrate_gyro(64);
  printf("GYRO cal=%d\r\n", gyro_cal_ok);

  gesture_init();

  if (mpu_read_reg(0x6B, &pwr)) {
    printf("PWR_MGMT_1 after init=0x%02X\r\n", pwr);
  }

  return gyro_cal_ok;
}

bool app_start_tasks(void) {
  BaseType_t control_created;
  BaseType_t uart_created;
  BaseType_t oled_created = pdPASS;

  control_created = xTaskCreate(app_control_task,
                                "Control",
                                APP_CONTROL_TASK_STACK_WORDS,
                                NULL,
                                APP_CONTROL_TASK_PRIORITY,
                                &s_control_task);

  if (control_created == pdPASS) {
    button_irq_set_notify_task(s_control_task);
  }

  uart_created = xTaskCreate(app_uart_task,
                             "UART",
                             APP_UART_TASK_STACK_WORDS,
                             NULL,
                             APP_UART_TASK_PRIORITY,
                             &s_uart_task);

  if (s_oled_ready) {
    oled_created = xTaskCreate(app_oled_task,
                               "OLED",
                               APP_OLED_TASK_STACK_WORDS,
                               NULL,
                               APP_OLED_TASK_PRIORITY,
                               &s_oled_task);
  }

  return control_created == pdPASS &&
         uart_created == pdPASS &&
         oled_created == pdPASS;
}

void app_update(uint32_t now) {
  mpu_motion_t motion;
  mpu_motion_scaled_t scaled;
  app_control_sample_t sample;
  bool motion_ok;
  bool recenter_requested;

  if (!app_i2c_take()) {
    printf("I2C mutex failed\r\n");
    return;
  }

  motion_ok = mpu_read_motion_raw(&motion);
  app_i2c_give();

  if (!motion_ok) {
    printf("MPU motion read failed\r\n");
    return;
  }

  if (!mpu_scale_motion(&motion, &scaled) ||
      !gesture_update_control(&scaled, &sample.control)) {
    printf("Control update failed\r\n");
    return;
  }

  sample.tick_ms = now;

  recenter_requested = ulTaskNotifyTake(pdTRUE, 0) > 0U;
  if (recenter_requested &&
      (!s_recenter_seen ||
       ((TickType_t) now - s_last_recenter_tick) >=
         pdMS_TO_TICKS(APP_RECENTER_DEBOUNCE_MS))) {
    s_recenter_seen = true;
    s_last_recenter_tick = (TickType_t) now;
    gesture_recenter();
    (void) gesture_update_control(&scaled, &sample.control);

    if (s_oled_task != NULL) {
      (void) xTaskNotifyGive(s_oled_task);
    }

    printf("RECENTER\r\n");
  }

  if (s_control_queue != NULL) {
    (void) xQueueOverwrite(s_control_queue, &sample);
  }
}
