#include "Buzzer.h"
#include "SystemTimer.h"

void Buzzer::init() {
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
    GPIOC->CRH &= ~(0xF << 28);
    GPIOC->CRH |= (0x3 << 28); 
    
    remainingBeeps = 0;
    isPulsing = false;
    off();
}

void Buzzer::on() {
    GPIOC->BSRR = (1 << 15);
}

void Buzzer::off() {
    GPIOC->BRR = (1 << 15);
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