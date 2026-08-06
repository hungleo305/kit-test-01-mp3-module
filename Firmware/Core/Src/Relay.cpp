#include "Relay.h"

void Relay::init() {
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
    GPIOC->CRL &= ~(0xF << 28);    
    GPIOC->CRL |=  (0x3 << 28);    

    off();
}

void Relay::on() {
    GPIOC->BSRR = (1 << 7);         
}

void Relay::off() {
    GPIOC->BRR  = (1 << 7);         
}

void Relay::toggle() {
    GPIOC->ODR ^= (1 << 7);
}