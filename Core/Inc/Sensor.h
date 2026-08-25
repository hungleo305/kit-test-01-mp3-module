#ifndef SENSOR_H
#define SENSOR_H

#include "stm32f1xx.h"

class Sensor {
private:
    GPIO_TypeDef* port;
    uint16_t pin;
    uint32_t lastInterruptTick; 

    volatile bool isDiagnosticMode;
    volatile uint32_t lastEnterTick;
    volatile uint32_t lastExitTick;
    volatile uint32_t blockDuration;
    volatile uint32_t gapDuration;
    volatile uint32_t prevBlockDuration;
    volatile uint32_t prevGapDuration;
    volatile bool newDataReady;

public:
    // [NEW] Task 3: Ch?c nãng ch?n ðoán c?m bi?n th?c t? (Logic Analyzer)
    void enableDiagnosticMode(bool enable);
    bool getDiagnosticData(uint16_t& currBlock, uint16_t& currGap, uint16_t& prevBlock, uint16_t& prevGap);
    Sensor(GPIO_TypeDef* port, uint16_t pin);
    void (*onDetectCallback)();
    void setCallback(void (*cb)());
    volatile bool isDetected;

    void init();
    void handleInterrupt(uint16_t extiPin);
};

#endif


