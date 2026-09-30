#ifndef RTC_H
#define RTC_H

#include "stm32f1xx.h"
#include "I2cBus.h"

class Rtc {
private:
    I2cBus* i2c_bus;
    uint8_t devAddr;

    uint8_t decToBcd(uint8_t val);
    uint8_t bcdToDec(uint8_t val);

public:
    Rtc(I2cBus* i2c_bus, uint8_t devAddr = 0xD0);
    void init();
    void setTime(uint8_t hour, uint8_t minute, uint8_t second);
    void getTime(uint8_t &hour, uint8_t &minute, uint8_t &second);
    void setDate(uint8_t dayOfWeek, uint8_t date, uint8_t month, uint8_t year);
    void getDate(uint8_t &dayOfWeek, uint8_t &date, uint8_t &month, uint8_t &year);
    bool isTimeLost();
};

#endif
