#ifndef RTC_H
#define RTC_H

#include "stm32f1xx.h"

class Rtc {
private:
    I2C_TypeDef* i2c;
    GPIO_TypeDef* sclPort;
    uint16_t sclPin;
    GPIO_TypeDef* sdaPort;
    uint16_t sdaPin;

    uint8_t decToBcd(uint8_t val);
    uint8_t bcdToDec(uint8_t val);
    void i2cWrite(uint8_t reg, uint8_t data);
    uint8_t i2cRead(uint8_t reg);

public:
    Rtc(I2C_TypeDef* i2c, GPIO_TypeDef* sclPort, uint16_t sclPin, GPIO_TypeDef* sdaPort, uint16_t sdaPin);
    void init();
    void setTime(uint8_t hour, uint8_t minute, uint8_t second);
    void getTime(uint8_t &hour, uint8_t &minute, uint8_t &second);
    void setDate(uint8_t dayOfWeek, uint8_t date, uint8_t month, uint8_t year);
    void getDate(uint8_t &dayOfWeek, uint8_t &date, uint8_t &month, uint8_t &year);
    bool isTimeLost();
};

#endif
