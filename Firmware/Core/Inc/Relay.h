#ifndef RELAY_H
#define RELAY_H

#include "stm32f1xx.h"

class Relay {
private:
    bool state;          

public:
    void init();
    void on();
    void off();
    void toggle();
    bool isOn();        
};

#endif