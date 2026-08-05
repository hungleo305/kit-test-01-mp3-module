#ifndef RELAY_H
#define RELAY_H

#include "stm32f1xx.h"

class Relay {
public:
    void init();
    void on();
    void off();
    void toggle(); 
};

#endif