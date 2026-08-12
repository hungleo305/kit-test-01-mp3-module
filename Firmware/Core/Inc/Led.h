#ifndef LED_H
#define LED_H

#include "stm32f1xx.h"

class Led {
private:
    GPIO_TypeDef* port;
    uint16_t pin;
public:
    Led(GPIO_TypeDef* port, uint16_t pin);
    void init();
    void toggle();
};

#endif