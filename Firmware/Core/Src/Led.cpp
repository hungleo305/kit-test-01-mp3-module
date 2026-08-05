#include "Led.h"

void Led::init() {
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
    GPIOB->CRL &= ~(0xF << 8);
    GPIOB->CRL |= (0x3 << 8);
}

void Led::toggle() {
    GPIOB->ODR ^= (1 << 2);
}