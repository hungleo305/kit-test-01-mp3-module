#ifndef RELAY_H
#define RELAY_H

#include "stm32f1xx.h"

class Relay {
private:
    GPIO_TypeDef* port;
    uint16_t pin;
    bool state;          

public:
    Relay(GPIO_TypeDef* port, uint16_t pin);
    void init();
    void on();
    void off();
    void toggle();
    bool isOn();        
};

#endif