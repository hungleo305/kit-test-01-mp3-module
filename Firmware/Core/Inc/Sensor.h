#ifndef SENSOR_H
#define SENSOR_H

#include "stm32f1xx.h"

class Sensor {
private:
    static uint32_t lastInterruptTick; 

public:
    static volatile uint16_t productCount;
    static volatile bool isDetected;

    void init();
    static void handleInterrupt();
};

#endif