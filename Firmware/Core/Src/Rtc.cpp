#include "Rtc.h"

bool Rtc::waitForLastTask() {
    uint32_t timeout = 0xFFFFF;
    while (((RTC->CRL & RTC_CRL_RTOFF) == 0) && (timeout > 0)) {
        timeout--;
    }
    return (timeout > 0);
}

void Rtc::init() {
    RCC->APB1ENR |= RCC_APB1ENR_PWREN | RCC_APB1ENR_BKPEN;
    PWR->CR |= PWR_CR_DBP;
    
    RCC->CSR |= RCC_CSR_LSION;
    uint32_t timeout_lsi = 0x1FFFFF; 
    while (((RCC->CSR & RCC_CSR_LSIRDY) == 0) && (timeout_lsi > 0)) {
        timeout_lsi--;
    }
    
    if (BKP->DR1 != 0x5050) {
        RCC->BDCR |= RCC_BDCR_BDRST;
        RCC->BDCR &= ~RCC_BDCR_BDRST;
        
        if (timeout_lsi > 0) {
            RCC->BDCR |= RCC_BDCR_RTCSEL_1;
            RCC->BDCR &= ~RCC_BDCR_RTCSEL_0;
            RCC->BDCR |= RCC_BDCR_RTCEN;
            
            RTC->CRL &= ~RTC_CRL_RSF;
            uint32_t sync_timeout = 0xFFFF;
            while (((RTC->CRL & RTC_CRL_RSF) == 0) && (sync_timeout > 0)) {
                sync_timeout--;
            }

            if (waitForLastTask()) {
                RTC->CRL |= RTC_CRL_CNF;
                RTC->PRLH = 0x0000;
                RTC->PRLL = 39999;
                RTC->CRL &= ~RTC_CRL_CNF;
                waitForLastTask();
                BKP->DR1 = 0x5050; 
            }
        }
    } else {
        RTC->CRL &= ~RTC_CRL_RSF;
        uint32_t sync_timeout = 0xFFFF;
        while (((RTC->CRL & RTC_CRL_RSF) == 0) && (sync_timeout > 0)) {
            sync_timeout--;
        }
        waitForLastTask();
    }
}
void Rtc::setTime(uint8_t hour, uint8_t minute, uint8_t second) {
    if (hour > 23) hour = 0;
    if (minute > 59) minute = 0;
    if (second > 59) second = 0;

    uint32_t counter = (hour * 3600) + (minute * 60) + second;
    
    PWR->CR |= PWR_CR_DBP; 
    
    if (waitForLastTask()) {
        RTC->CRL |= RTC_CRL_CNF; 
        RTC->CNTH = counter >> 16;
        RTC->CNTL = counter & 0xFFFF;
        RTC->CRL &= ~RTC_CRL_CNF; 
        waitForLastTask();
    }
}

void Rtc::getTime(uint8_t &hour, uint8_t &minute, uint8_t &second) {
    uint32_t high1, low, high2;
    
    high1 = RTC->CNTH;
    low = RTC->CNTL;
    high2 = RTC->CNTH;
    
    if (high1 != high2) {
        high1 = RTC->CNTH;
        low = RTC->CNTL;
    }
    
    uint32_t counter = (high1 << 16) | low;
    counter = counter % 86400; 
    
    hour = counter / 3600;
    minute = (counter % 3600) / 60;
    second = (counter % 3600) % 60;
}