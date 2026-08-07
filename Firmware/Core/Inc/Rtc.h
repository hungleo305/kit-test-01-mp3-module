#ifndef RTC_H
#define RTC_H

#include "stm32f1xx.h"

class Rtc {
public:
    void init();
    void setTime(uint8_t hour, uint8_t minute, uint8_t second);
    void getTime(uint8_t &hour, uint8_t &minute, uint8_t &second);
    bool isTimeLost();
    
private:
    uint8_t decToBcd(uint8_t val);
    uint8_t bcdToDec(uint8_t val);
    void i2cWrite(uint8_t reg, uint8_t data);
    uint8_t i2cRead(uint8_t reg);
};

#endif