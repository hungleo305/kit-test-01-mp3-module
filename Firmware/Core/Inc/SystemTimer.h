#ifndef SYSTEM_TIMER_H
#define SYSTEM_TIMER_H

#include "stm32f1xx.h"

class SystemTimer {
public:
    static uint32_t getTick();
    static void delay_ms(uint32_t ms);
};

#endif