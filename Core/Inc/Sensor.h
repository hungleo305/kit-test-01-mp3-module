#ifndef SENSOR_H
#define SENSOR_H

#include "stm32f1xx.h"

class Sensor {
private:
    GPIO_TypeDef* port;
    uint16_t pin;
    uint32_t lastInterruptTick; 

    

public:
            Sensor(GPIO_TypeDef* port, uint16_t pin);
    void (*onDetectCallback)();
    void setCallback(void (*cb)());
    volatile bool isDetected;

    enum TriggerEdge {
        RISING_EDGE,
        FALLING_EDGE,
        BOTH_EDGES
    };
    void init(TriggerEdge edge = FALLING_EDGE);
    void handleInterrupt(uint16_t extiPin);
};

#endif


