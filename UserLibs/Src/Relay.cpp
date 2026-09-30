#include "Relay.h"

Relay::Relay(GPIO_TypeDef* port, uint16_t pin) : port(port), pin(pin) {}

void Relay::init() {
    if (port == GPIOA) RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
    else if (port == GPIOB) RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
    else if (port == GPIOC) RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
    else if (port == GPIOD) RCC->APB2ENR |= RCC_APB2ENR_IOPDEN;

    uint32_t pinIndex = 0;
    while (!(pin & (1 << pinIndex))) pinIndex++;

    if (pinIndex < 8) {
        port->CRL &= ~(0xF << (pinIndex * 4));
        port->CRL |= (0x3 << (pinIndex * 4));
    } else {
        port->CRH &= ~(0xF << ((pinIndex - 8) * 4));
        port->CRH |= (0x3 << ((pinIndex - 8) * 4));
    }
    
    state = false;
    off();
}

void Relay::on() {
    state = true;
    port->BSRR = pin;
}

void Relay::off() {
    state = false;
    port->BRR = pin;
}

void Relay::toggle() {
    state = !state;
    if (state) port->BSRR = pin;
    else port->BRR = pin;
}

bool Relay::isOn() {
    return state;
}