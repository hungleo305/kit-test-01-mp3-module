#ifndef RTC_H
#define RTC_H

#include "stm32f1xx.h"

class Rtc {
public:
    void init();
    void setTime(uint8_t hour, uint8_t minute, uint8_t second);
    void getTime(uint8_t &hour, uint8_t &minute, uint8_t &second);
    
private:
    bool waitForLastTask();
};

#endif