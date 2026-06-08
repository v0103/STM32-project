#pragma once

#include <stdint.h>

/* STM32F103C8 runs the Cortex-M3 core and SysTick at 72 MHz. */
#define configCPU_CLOCK_HZ                      72000000UL
#define configTICK_RATE_HZ                      1000U
#define configTICK_TYPE_WIDTH_IN_BITS           TICK_TYPE_WIDTH_32_BITS

/* Scheduler configuration. */
#define configUSE_PREEMPTION                    1
#define configUSE_TIME_SLICING                  1
#define configUSE_TICKLESS_IDLE                 0
#define configMAX_PRIORITIES                    5
#define configMINIMAL_STACK_SIZE                128U
#define configMAX_TASK_NAME_LEN                 16
#define configIDLE_SHOULD_YIELD                 1

/* The Blue Pill has 20 KB SRAM. Start with a 10 KB RTOS heap. */
#define configSUPPORT_DYNAMIC_ALLOCATION        1
#define configSUPPORT_STATIC_ALLOCATION         0
#define configTOTAL_HEAP_SIZE                   (10U * 1024U)
#define configAPPLICATION_ALLOCATED_HEAP        0

/* Synchronisation features needed by the later multi-task design. */
#define configUSE_MUTEXES                       1
#define configUSE_RECURSIVE_MUTEXES             0
#define configUSE_COUNTING_SEMAPHORES           0
#define configQUEUE_REGISTRY_SIZE               0

/* Features not needed during the first RTOS integration step. */
#define configUSE_TIMERS                        0
#define configUSE_EVENT_GROUPS                  0
#define configUSE_STREAM_BUFFERS                0
#define configUSE_CO_ROUTINES                   0
#define configUSE_TRACE_FACILITY                0
#define configUSE_STATS_FORMATTING_FUNCTIONS    0
#define configGENERATE_RUN_TIME_STATS           0
#define configUSE_IDLE_HOOK                     0
#define configUSE_TICK_HOOK                     0
#define configUSE_MALLOC_FAILED_HOOK            0
#define configCHECK_FOR_STACK_OVERFLOW          0
#define configUSE_NEWLIB_REENTRANT              0

/* APIs used by the first application task. */
#define INCLUDE_xTaskDelayUntil                 1
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_xTaskGetSchedulerState          1
#define INCLUDE_xTaskGetCurrentTaskHandle       1
#define INCLUDE_uxTaskGetStackHighWaterMark     1

/*
 * STM32F103 implements four NVIC priority bits. Lower numeric values mean
 * higher interrupt urgency. ISRs that call FreeRTOS FromISR APIs must use a
 * numeric priority of 5 through 15.
 */
#define configPRIO_BITS                         4
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY 15
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5
#define configKERNEL_INTERRUPT_PRIORITY         \
  (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    \
  (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))

/* Connect the Cortex-M3 FreeRTOS port to the STM32 vector table names. */
#define vPortSVCHandler                         SVC_Handler
#define xPortPendSVHandler                      PendSV_Handler
#define xPortSysTickHandler                     SysTick_Handler
#define configCHECK_HANDLER_INSTALLATION        1

#define configASSERT(condition)                  \
  do {                                           \
    if ((condition) == 0) {                      \
      __asm volatile ("cpsid i" ::: "memory");   \
      for (;;) {                                 \
      }                                          \
    }                                            \
  } while (0)
