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
    
    uint8_t segMap[8];
    uint8_t displayBuffer[16]; 
    uint32_t dotBuffer; 
    uint32_t lastScanTick;
    uint8_t currentScanBit; 

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
                
    // Ánh xạ lại chân segment (khi module đấu lệch thứ tự A..DP).
    void setSegmentMap(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t e, uint8_t f, uint8_t g, uint8_t dp);
    
    // Đảo vị trí các digit (bit i = digit i bị đảo ngược vật lý).
    void setReverseMask(uint32_t mask);
    
    // Đảo cực byte hiển thị (module LED âm cực).
    void setInverted(bool state);
    
    // Tắt hoàn toàn màn hình (đồng nghĩa với blank())
    void hide();
    
    // Khởi tạo chân GPIO. STROBE được đưa lên mức HIGH (mức nghỉ).
    void init();

    // Xoá buffer hiển thị
    void clearBuffer();
    
    // Đặt giá trị cho 1 chữ số
    void setDigit(uint8_t index, uint8_t value, bool showDot = false);
    
    // Điều khiển dấu chấm
    void setDot(uint8_t index, bool showDot);
    
    // In số ra màn hình
    void setNumber(uint32_t number, uint8_t startIndex, uint8_t length, bool padZeroes = false);
    
    // Đẩy buffer ra màn hình
    void refresh();
    
    // Xoá sạch màn hình lập tức
    void blank();
    
    // Hiện gạch ngang '----' để test
    void showDashes();
    
    // Quét từng segment để test phần cứng
    void autoScan(uint32_t delayMs = 1000);
};

#endif