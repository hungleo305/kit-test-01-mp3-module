#ifndef LED_H
#define LED_H

#include "stm32f1xx.h"

class Led {
public:
    void init();
    void toggle();
};

#endif