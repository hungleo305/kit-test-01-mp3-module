#include "Rtc.h"

uint8_t Rtc::decToBcd(uint8_t val) {
    return ((val / 10) << 4) | (val % 10);
}

uint8_t Rtc::bcdToDec(uint8_t val) {
    return ((val >> 4) * 10) + (val & 0x0F);
}

void Rtc::init() {
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN | RCC_APB2ENR_AFIOEN;
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;

    GPIOB->CRL &= 0x00FFFFFFU; 
    GPIOB->CRL |= 0xFF000000U; 
    
    GPIOB->BSRR = (1 << 6) | (1 << 7);

    I2C1->CR1 |= I2C_CR1_SWRST;
    I2C1->CR1 &= ~I2C_CR1_SWRST;

    I2C1->CR2 = 36;
    I2C1->CCR = 180;        
    I2C1->TRISE = 37;        
    I2C1->CR1 |= I2C_CR1_PE; 
}

void Rtc::i2cWrite(uint8_t reg, uint8_t data) {
    uint32_t t = 10000; // Bi?n ch?ng treo MCU

    while ((I2C1->SR2 & I2C_SR2_BUSY) && --t); if(!t) return;

    I2C1->CR1 |= I2C_CR1_START;                
    t = 10000; while (!(I2C1->SR1 & I2C_SR1_SB) && --t); if(!t) return;

    I2C1->DR = 0xD0;                            
    t = 10000; while (!(I2C1->SR1 & I2C_SR1_ADDR) && --t); if(!t) return;
    (void)I2C1->SR2;                            

    I2C1->DR = reg;                            
    t = 10000; while (!(I2C1->SR1 & I2C_SR1_TXE) && --t); if(!t) return;

    I2C1->DR = data;                        
    t = 10000; while (!(I2C1->SR1 & I2C_SR1_BTF) && --t); if(!t) return;

    I2C1->CR1 |= I2C_CR1_STOP;                
}

uint8_t Rtc::i2cRead(uint8_t reg) {
    uint8_t data = 0;
    uint32_t t = 10000;

    while ((I2C1->SR2 & I2C_SR2_BUSY) && --t); if(!t) return 0;

    I2C1->CR1 |= I2C_CR1_START;                 
    t = 10000; while (!(I2C1->SR1 & I2C_SR1_SB) && --t); if(!t) return 0;

    I2C1->DR = 0xD0;                            
    t = 10000; while (!(I2C1->SR1 & I2C_SR1_ADDR) && --t); if(!t) return 0;
    (void)I2C1->SR2;                            

    I2C1->DR = reg;                            
    t = 10000; while (!(I2C1->SR1 & I2C_SR1_TXE) && --t); if(!t) return 0;

    I2C1->CR1 |= I2C_CR1_START;                 
    t = 10000; while (!(I2C1->SR1 & I2C_SR1_SB) && --t); if(!t) return 0;

    I2C1->DR = 0xD1;                            
    t = 10000; while (!(I2C1->SR1 & I2C_SR1_ADDR) && --t); if(!t) return 0;

    I2C1->CR1 &= ~I2C_CR1_ACK;                  
    (void)I2C1->SR2;                         

    I2C1->CR1 |= I2C_CR1_STOP;                  
    t = 10000; while (!(I2C1->SR1 & I2C_SR1_RXNE) && --t); if(!t) return 0;

    data = I2C1->DR;                            
    return data;
}

void Rtc::setTime(uint8_t hour, uint8_t minute, uint8_t second) {
    i2cWrite(0x00, decToBcd(second) & 0x7F); 
    i2cWrite(0x01, decToBcd(minute));
    i2cWrite(0x02, decToBcd(hour) & 0x3F);      
}

void Rtc::getTime(uint8_t &hour, uint8_t &minute, uint8_t &second) {
    second = bcdToDec(i2cRead(0x00) & 0x7F);
    minute = bcdToDec(i2cRead(0x01));
    hour   = bcdToDec(i2cRead(0x02) & 0x3F);
}
bool Rtc::isTimeLost() {
    uint8_t secReg = i2cRead(0x00);
    
    if (secReg & 0x80) {
        return true;
    }
    
    return false;
}