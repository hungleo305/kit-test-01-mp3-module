#ifndef BUZZER_H
#define BUZZER_H

#include "stm32f1xx.h"

class Buzzer {
private:
    uint32_t lastTick;
    uint32_t tOn;
    uint32_t tOff;
    int remainingBeeps;
    bool isPulsing;

public:
    void init();
    void on();
    void off();
    void beep(int times, int timeOn, int timeOff);
    void update();
};

#endif