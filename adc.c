#include "adc.h"
#include "hal.h"

void adc_init(void){
    RCC->CFGR &= ~(3U << 14);
    RCC->CFGR |= (2U << 14);

    RCC->APB2ENR |= BIT(9);

    ADC1->CR2 |= BIT(0);
    spin(1000);

    ADC1->CR2 |= BIT(3);           // RSTCAL
    while (ADC1->CR2 & BIT(3)) {
    }

    ADC1->CR2 |= BIT(2);           // CAL
    while (ADC1->CR2 & BIT(2)) {
    }
}
void adc_config_channel(uint8_t channel) {
  uint32_t sample_time = 7; // 239.5 cycles, the longest sample time, for maximum accuracy

  if (channel <= 9) {
    uint32_t shift = (uint32_t) channel * 3U;

    ADC1->SMPR2 &= ~(7U << shift);
    ADC1->SMPR2 |=  (sample_time << shift);
  } else {
    uint32_t shift = ((uint32_t) channel - 10U) * 3U;

    ADC1->SMPR1 &= ~(7U << shift);
    ADC1->SMPR1 |=  (sample_time << shift);
  }
}
uint16_t adc_read(uint8_t channel){
    ADC1->SQR1 = 0;
    ADC1->SQR3 = channel;

    ADC1->CR2 |= BIT(0);

    while ((ADC1->SR & BIT(1)) == 0) {
    }
    return (uint16_t) ADC1->DR;
}