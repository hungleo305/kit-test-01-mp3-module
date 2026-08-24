#ifndef BUZZER_H
#define BUZZER_H

#include "stm32f1xx.h"

#define MAX_BEEP_QUEUE 5

struct BeepReq {
    uint8_t times;
    uint16_t tOn;
    uint16_t tOff;
};

class Buzzer {
private:
    GPIO_TypeDef* port;
    uint16_t pin;
    uint32_t lastTick;
    uint32_t tOn;
    uint32_t tOff;
    int remainingBeeps;
    bool isPulsing;
    
    BeepReq queue[MAX_BEEP_QUEUE];
    uint8_t head;
    uint8_t tail;
    uint8_t count;
    
    void playNext();

public:
    Buzzer(GPIO_TypeDef* port, uint16_t pin);
    void init();
    void on();
    void off();
    void beep(int times, int timeOn, int timeOff, bool instant = false);
    void update();
};

#endif
