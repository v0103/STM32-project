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
  APP_CONTROL_PERIOD_MS = 100,
  APP_OLED_PERIOD_MS = 500,
  APP_CONTROL_TASK_STACK_WORDS = 512,
  APP_OLED_TASK_STACK_WORDS = 384,
  APP_CONTROL_TASK_PRIORITY = tskIDLE_PRIORITY + 2,
  APP_OLED_TASK_PRIORITY = tskIDLE_PRIORITY + 1,
};

static bool s_oled_ready;
static QueueHandle_t s_control_queue;
static SemaphoreHandle_t s_i2c_mutex;

static bool app_i2c_take(void) {
  return s_i2c_mutex != NULL &&
         xSemaphoreTake(s_i2c_mutex, portMAX_DELAY) == pdTRUE;
}

static void app_i2c_give(void) {
  if (s_i2c_mutex != NULL) {
    (void) xSemaphoreGive(s_i2c_mutex);
  }
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
  TickType_t last_wake;
  gesture_control_t control;

  (void) argument;
  last_wake = xTaskGetTickCount();

  for (;;) {
    vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(APP_OLED_PERIOD_MS));

    if (s_oled_ready &&
        xQueuePeek(s_control_queue, &control, 0) == pdPASS &&
        app_i2c_take()) {
      (void) oled_show_control(&control);
      app_i2c_give();
    }
  }
}

bool app_init(void) {
  bool mpu_ok;
  bool gyro_cal_ok;
  uint8_t pwr = 0;

  s_control_queue = xQueueCreate(1, sizeof(gesture_control_t));
  s_i2c_mutex = xSemaphoreCreateMutex();
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
  BaseType_t oled_created = pdPASS;

  control_created = xTaskCreate(app_control_task,
                                "Control",
                                APP_CONTROL_TASK_STACK_WORDS,
                                NULL,
                                APP_CONTROL_TASK_PRIORITY,
                                NULL);

  if (s_oled_ready) {
    oled_created = xTaskCreate(app_oled_task,
                               "OLED",
                               APP_OLED_TASK_STACK_WORDS,
                               NULL,
                               APP_OLED_TASK_PRIORITY,
                               NULL);
  }

  return control_created == pdPASS && oled_created == pdPASS;
}

void app_update(uint32_t now) {
  mpu_motion_t motion;
  mpu_motion_scaled_t scaled;
  gesture_control_t control;
  bool motion_ok;

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
      !gesture_update_control(&scaled, &control)) {
    printf("Control update failed\r\n");
    return;
  }

  if (button_take_recenter_request(now)) {
    gesture_recenter();
    (void) gesture_update_control(&scaled, &control);

    if (s_oled_ready && app_i2c_take()) {
      (void) oled_show_recenter();
      app_i2c_give();
    }

    printf("RECENTER\r\n");
  }

  control_packet_print(now, &control);

  if (s_control_queue != NULL) {
    (void) xQueueOverwrite(s_control_queue, &control);
  }
}
