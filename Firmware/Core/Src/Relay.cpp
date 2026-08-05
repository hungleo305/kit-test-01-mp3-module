#include "Relay.h"

void Relay::init() {
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;

    GPIOC->CRL &= ~(0xF << 16);    
    GPIOC->CRL |=  (0x3 << 16);     

    off();                      
}

void Relay::on() {
    GPIOC->BRR = (1 << 4);         
}

void Relay::off() {
    GPIOC->BSRR  = (1 << 4);         
}

void Relay::toggle() {
    GPIOC->ODR ^= (1 << 4);
}