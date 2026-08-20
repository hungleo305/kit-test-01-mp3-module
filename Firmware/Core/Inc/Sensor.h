#ifndef SENSOR_H
#define SENSOR_H

#include "stm32f1xx.h"

class Sensor {
private:
    GPIO_TypeDef* port;
    uint16_t pin;
    static uint32_t lastInterruptTick; 

    static volatile bool isDiagnosticMode;
    static volatile uint32_t lastEnterTick;
    static volatile uint32_t lastExitTick;
    static volatile uint32_t blockDuration;
    static volatile uint32_t gapDuration;
    static volatile bool newDataReady;

public:
    // [NEW] Task 3: Chức năng chẩn đoán cảm biến thực tế (Logic Analyzer)
    static void enableDiagnosticMode(bool enable);
    static bool getDiagnosticData(uint32_t& outBlockDuration, uint32_t& outGapDuration);
    Sensor(GPIO_TypeDef* port, uint16_t pin);
    static volatile uint16_t productCount;
    static volatile bool isDetected;

    void init();
    static void handleInterrupt(uint16_t extiPin);
};

#endif