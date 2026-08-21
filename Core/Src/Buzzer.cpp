#include "Buzzer.h"
#include "SystemTimer.h"

Buzzer::Buzzer(GPIO_TypeDef* port, uint16_t pin) : port(port), pin(pin) {}

void Buzzer::init() {
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
    
    remainingBeeps = 0;
    isPulsing = false;
    off();
}

void Buzzer::on() {
    port->BSRR = pin;
}

void Buzzer::off() {
    port->BRR = pin;
}

void Buzzer::beep(int times, int timeOn, int timeOff) {
    if (times > 0) {
        remainingBeeps = times;
        tOn = timeOn;
        tOff = timeOff;
        isPulsing = true;
        lastTick = SystemTimer::getTick();
        on();
    }
}

void Buzzer::update() {
    if (remainingBeeps == 0) return;

    uint32_t currentTick = SystemTimer::getTick();

    if (isPulsing) {
        if (currentTick - lastTick >= tOn) {
            off();
            isPulsing = false;
            lastTick = currentTick;
        }
    } else {
        if (currentTick - lastTick >= tOff) {
            remainingBeeps--;
            if (remainingBeeps > 0) {
                on();
                isPulsing = true;
                lastTick = currentTick;
            }
        }
    }
}