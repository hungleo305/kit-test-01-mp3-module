#ifndef SENSOR_H
#define SENSOR_H

#include "stm32f1xx.h"

class Sensor {
private:
    GPIO_TypeDef* port;
    uint16_t pin;
    static uint32_t lastInterruptTick; 

public:
    Sensor(GPIO_TypeDef* port, uint16_t pin);
    static volatile uint16_t productCount;
    static volatile bool isDetected;

    void init();
    static void handleInterrupt(uint16_t extiPin);
    
    // Hàm stress-test xem MCU có thể bắt được bao nhiêu ngắt cảm biến trong 1 giây
    uint32_t testMaxCountsPerSecond();
};

#endif