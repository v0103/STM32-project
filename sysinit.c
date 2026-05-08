#include "hal.h"

uint32_t SystemCoreClock = FREQ;

void SystemInit(void) {
  // 1. Flash: 72MHz needs 2 wait states, enable prefetch
  FLASH->ACR &= ~FLASH_ACR_LATENCY;
  FLASH->ACR |= FLASH_ACR_PRFTBE | FLASH_ACR_LATENCY_1;

  // 2. Enable HSE and wait until it is stable
  RCC->CR |= RCC_CR_HSEON;
  while ((RCC->CR & RCC_CR_HSERDY) == 0) spin(1);

  // 3. Configure PLL and APB prescalers
  RCC->CFGR |= RCC_CFGR_PLLSRC;       // PLL source = HSE
  RCC->CFGR |= RCC_CFGR_PLLMULL9;     // HSE 8MHz * 9 = 72MHz
  RCC->CFGR |= RCC_CFGR_PPRE1_DIV2;   // APB1 = 36MHz

  // 4. Enable PLL and wait until ready
  RCC->CR |= RCC_CR_PLLON;
  while ((RCC->CR & RCC_CR_PLLRDY) == 0) spin(1);

  // 5. Switch SYSCLK to PLL and wait until selected
  RCC->CFGR |= RCC_CFGR_SW_PLL;
  while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL) spin(1);

  SystemCoreClock = FREQ;
}
