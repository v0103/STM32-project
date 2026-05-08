#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "stm32f103xb.h"

#define FREQ 72000000UL
#define APB1_FREQ 36000000UL
#define APB2_FREQ 72000000UL

#define BIT(x) (1UL << (x))
#define PIN(bank, num) ((((bank) - 'A') << 8) | (num))
#define PINNO(pin) (pin & 255)
#define PINBANK(pin) (pin >> 8)

#define GPIO_CFG_IN_ANALOG 0x0U
#define GPIO_CFG_IN_FLOATING 0x4U
#define GPIO_CFG_IN_PULL 0x8U
#define GPIO_CFG_OUT_PP_2MHZ 0x2U
#define GPIO_CFG_AF_PP_2MHZ 0xAU

static inline GPIO_TypeDef *gpio_bank(uint8_t bank) {
  return (GPIO_TypeDef *)(GPIOA_BASE + 0x400UL * bank);
}

static inline void systick_init(uint32_t ticks) {
  if ((ticks - 1) > 0xffffff) return;  // Systick timer is 24 bit
  SysTick->LOAD = ticks - 1;
  SysTick->VAL = 0;
  SysTick->CTRL = BIT(0) | BIT(1) | BIT(2);  // Enable systick
}
// t: expiration time, prd: period, now: current time. Return true if expired
static inline bool timer_expired(uint32_t *t, uint32_t prd, uint32_t now) {
  if (now + prd < *t) *t = 0;                    // Time wrapped? Reset timer
  if (*t == 0) *t = now + prd;                   // First poll? Set expiration
  if (*t > now) return false;                    // Not expired yet, return
  *t = (now - *t) > prd ? now + prd : *t + prd;  // Next expiration time
  return true;                                   // Expired, return true
}


static inline void gpio_set_mode(uint16_t pin, uint8_t mode) {
  GPIO_TypeDef *gpio = gpio_bank((uint8_t) PINBANK(pin));
  int n = PINNO(pin);
  volatile uint32_t *reg = n < 8 ? &gpio->CRL : &gpio->CRH;
  int shift = (n & 7) * 4;

  RCC->APB2ENR |= BIT(PINBANK(pin) + 2);

  *reg &= ~(0xFU << shift);
  *reg |= ((uint32_t)(mode & 0xF) << shift);
}

static inline bool gpio_read(uint16_t pin) {
  GPIO_TypeDef *gpio = gpio_bank((uint8_t) PINBANK(pin));
  return (gpio->IDR & (1U << PINNO(pin))) != 0;
}
static inline void gpio_write(uint16_t pin, bool val) {
  GPIO_TypeDef *gpio = gpio_bank((uint8_t) PINBANK(pin));
  gpio->BSRR = (1U << PINNO(pin)) << (val ? 0 : 16);
}

static inline void spin(volatile uint32_t count) {
  while (count--) (void) 0;
}
static inline void uart_write_byte(USART_TypeDef *uart, uint8_t byte) {
  uart->DR = byte;
  while ((uart->SR & BIT(7)) == 0) spin(1);
}

static inline void uart_write_buf(USART_TypeDef *uart, const char *buf, size_t len) {
  while (len-- > 0) uart_write_byte(uart, (uint8_t) *buf++);
}
static inline void uart_init(USART_TypeDef *uart, unsigned long baud) {
  uint16_t tx = 0, rx = 0;
  unsigned long uart_freq = 0;

  if (uart == USART1) {
    RCC->APB2ENR |= BIT(2);   // GPIOA clock
    RCC->APB2ENR |= BIT(14);  // USART1 clock
    uart_freq = APB2_FREQ;

    tx = PIN('A', 9);
    rx = PIN('A', 10);
  }else if (uart == USART2) {
    RCC->APB2ENR |= BIT(2);   // GPIOA clock
    RCC->APB1ENR |= BIT(17);  // USART2 clock
    uart_freq = APB1_FREQ;

    tx = PIN('A', 2);
    rx = PIN('A', 3);
  }else if (uart == USART3) {
    RCC->APB2ENR |= BIT(3);   // GPIOB clock
    RCC->APB1ENR |= BIT(18);  // USART3 clock
    uart_freq = APB1_FREQ;

    tx = PIN('B', 10);
    rx = PIN('B', 11);
  }else{
    return;
  }

  gpio_set_mode(tx, GPIO_CFG_AF_PP_2MHZ);
  gpio_set_mode(rx, GPIO_CFG_IN_FLOATING);

  uart->CR1 = 0;
  uart->BRR = uart_freq / baud;
  uart->CR1 = BIT(13) | BIT(3) | BIT(2);     // UE, TE, RE
}
