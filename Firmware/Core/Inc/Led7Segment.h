#ifndef LED7SEGMENT_H
#define LED7SEGMENT_H

#include "main.h"
#include <stdint.h>

class Led7Segment {
private:
    uint16_t displayValue;
    bool colonState;

    void clockPulse();
    void sendBit(uint8_t bit);
    void sendByte(uint8_t data);
    void latch();

public:
    void init();
    void setValue(uint16_t val);
    void setColon(bool state);
    void update();
};

#endif