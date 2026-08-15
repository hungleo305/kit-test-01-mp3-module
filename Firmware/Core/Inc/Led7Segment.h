#ifndef LED7SEGMENT_H
#define LED7SEGMENT_H

#include "main.h"
#include <stdint.h>
#include "Relay.h"

class Led7Segment {
private:
    GPIO_TypeDef* dataPort;
    uint16_t dataPin;
    GPIO_TypeDef* clockPort;
    uint16_t clockPin;
    GPIO_TypeDef* strobePort;
    uint16_t strobePin;
    Relay* relay;
    
    uint16_t displayValue;
    bool colonState;

    void clockPulse();
    void sendBit(uint8_t bit);
    void sendByte(uint8_t data);
    void latch();

public:
    Led7Segment(GPIO_TypeDef* dataPort, uint16_t dataPin, 
                GPIO_TypeDef* clockPort, uint16_t clockPin, 
                GPIO_TypeDef* strobePort, uint16_t strobePin, 
                Relay* relay);
    void init();
    void setValue(uint16_t val);
    void setColon(bool state);
    void update(bool isTimeMode = false);
    void blank();     
    void restore();
    void turnOff();
    void turnOn();
};

#endif