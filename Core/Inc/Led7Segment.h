#ifndef LED7SEGMENT_H
#define LED7SEGMENT_H

#include "main.h"
#include <stdint.h>

class Led7Segment {
private:
    GPIO_TypeDef* dataPort;
    uint16_t dataPin;
    GPIO_TypeDef* clockPort;
    uint16_t clockPin;
    GPIO_TypeDef* strobePort;
    uint16_t strobePin;
    
    uint8_t numDigits;
    uint32_t revMask;  
    bool isInverted;
    
    // --- 1. BIẾN IS_OE CẦN THÊM VÀO ĐÂY ---
    bool isOE; 
    
    uint8_t segMap[8];
    uint8_t displayBuffer[16]; 
    uint32_t dotBuffer; 

    void clockPulse();
    void sendBit(uint8_t bit);
    void sendByte(uint8_t data);
    void latch();
    
    uint8_t getLogicalDigit(uint8_t digit);
    uint8_t mapLogicalToPhysical(uint8_t logicalSeg, bool isReversed);

public:
    Led7Segment(GPIO_TypeDef* dPort, uint16_t dPin, 
                GPIO_TypeDef* clkPort, uint16_t clkPin, 
                GPIO_TypeDef* strPort, uint16_t strPin,
                uint8_t digits);
                
    void setSegmentMap(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t e, uint8_t f, uint8_t g, uint8_t dp);
    void setReverseMask(uint32_t mask);
    void setInverted(bool state);
    
    // --- 2. HAI HÀM CẤU HÌNH OE CẦN THÊM VÀO ĐÂY ---
    void setStrobeOE(bool state);
    void hide();
    
    void init();

    void update(uint32_t value, bool padZeroes = false, uint32_t dotMask = 0);
    void updateSplit(uint32_t rightVal, uint8_t rightDigits, bool rightPad, uint32_t leftVal, uint8_t leftDigits, bool leftPad, uint32_t dotMask = 0);

    void clearBuffer();
    void setDigit(uint8_t index, uint8_t value, bool showDot = false);
    void setRaw(uint8_t index, uint8_t rawHex);
    void setNumber(uint32_t number, uint8_t startIndex, uint8_t length, bool padZeroes = false);
    
    void refresh();
    void blank();
    void showDashes();
};

#endif