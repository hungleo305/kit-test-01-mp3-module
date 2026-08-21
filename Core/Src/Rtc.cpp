#include "Rtc.h"

uint8_t Rtc::decToBcd(uint8_t val) {
    return ((val / 10) << 4) | (val % 10);
}

uint8_t Rtc::bcdToDec(uint8_t val) {
    return ((val >> 4) * 10) + (val & 0x0F);
}

Rtc::Rtc(I2C_TypeDef* i2c, GPIO_TypeDef* sclPort, uint16_t sclPin, GPIO_TypeDef* sdaPort, uint16_t sdaPin)
    : i2c(i2c), sclPort(sclPort), sclPin(sclPin), sdaPort(sdaPort), sdaPin(sdaPin) {}

void Rtc::init() {
    if (sclPort == GPIOA || sdaPort == GPIOA) RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
    if (sclPort == GPIOB || sdaPort == GPIOB) RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
    if (sclPort == GPIOC || sdaPort == GPIOC) RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
    if (sclPort == GPIOD || sdaPort == GPIOD) RCC->APB2ENR |= RCC_APB2ENR_IOPDEN;
    
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;
    
    if (i2c == I2C1) RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;
    else if (i2c == I2C2) RCC->APB1ENR |= RCC_APB1ENR_I2C2EN;

    GPIO_TypeDef* ports[] = {sclPort, sdaPort};
    uint16_t pins[] = {sclPin, sdaPin};
    
    for (int i = 0; i < 2; i++) {
        uint32_t pinIndex = 0;
        while (!(pins[i] & (1 << pinIndex))) pinIndex++;

        if (pinIndex < 8) {
            ports[i]->CRL &= ~(0xF << (pinIndex * 4));
            ports[i]->CRL |= (0xF << (pinIndex * 4)); // AF OD 50MHz
        } else {
            ports[i]->CRH &= ~(0xF << ((pinIndex - 8) * 4));
            ports[i]->CRH |= (0xF << ((pinIndex - 8) * 4));
        }
    }
    
    sclPort->BSRR = sclPin;
    sdaPort->BSRR = sdaPin;

    i2c->CR1 |= I2C_CR1_SWRST;
    i2c->CR1 &= ~I2C_CR1_SWRST;

    i2c->CR2 = 36;
    i2c->CCR = 180;        
    i2c->TRISE = 37;        
    i2c->CR1 |= I2C_CR1_PE; 
}

void Rtc::i2cWrite(uint8_t reg, uint8_t data) {
    uint32_t t = 10000; 

    while ((i2c->SR2 & I2C_SR2_BUSY) && --t); if(!t) return;

    i2c->CR1 |= I2C_CR1_START;                
    t = 10000; while (!(i2c->SR1 & I2C_SR1_SB) && --t); if(!t) return;

    i2c->DR = 0xD0;                            
    t = 10000; while (!(i2c->SR1 & I2C_SR1_ADDR) && --t); if(!t) return;
    (void)i2c->SR2;                            

    i2c->DR = reg;                            
    t = 10000; while (!(i2c->SR1 & I2C_SR1_TXE) && --t); if(!t) return;

    i2c->DR = data;                        
    t = 10000; while (!(i2c->SR1 & I2C_SR1_BTF) && --t); if(!t) return;

    i2c->CR1 |= I2C_CR1_STOP;                
}

uint8_t Rtc::i2cRead(uint8_t reg) {
    uint8_t data = 0;
    uint32_t t = 10000;

    while ((i2c->SR2 & I2C_SR2_BUSY) && --t); if(!t) return 0;

    i2c->CR1 |= I2C_CR1_START;                 
    t = 10000; while (!(i2c->SR1 & I2C_SR1_SB) && --t); if(!t) return 0;

    i2c->DR = 0xD0;                            
    t = 10000; while (!(i2c->SR1 & I2C_SR1_ADDR) && --t); if(!t) return 0;
    (void)i2c->SR2;                            

    i2c->DR = reg;                            
    t = 10000; while (!(i2c->SR1 & I2C_SR1_TXE) && --t); if(!t) return 0;

    i2c->CR1 |= I2C_CR1_START;                 
    t = 10000; while (!(i2c->SR1 & I2C_SR1_SB) && --t); if(!t) return 0;

    i2c->DR = 0xD1;                            
    t = 10000; while (!(i2c->SR1 & I2C_SR1_ADDR) && --t); if(!t) return 0;

    i2c->CR1 &= ~I2C_CR1_ACK;                  
    (void)i2c->SR2;                         

    i2c->CR1 |= I2C_CR1_STOP;                  
    t = 10000; while (!(i2c->SR1 & I2C_SR1_RXNE) && --t); if(!t) return 0;

    data = i2c->DR;                            
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
    // Đọc thử vài lần để tránh nhiễu I2C lúc khởi động
    for (int i = 0; i < 3; i++) {
        uint8_t secReg = i2cRead(0x00);
        // Nếu đọc được bit 7 == 0 (Clock đang chạy), thì kết luận là không lỗi
        if ((secReg & 0x80) == 0) {
            return false;
        }
        HAL_Delay(5);
    }
    // Nếu cả 3 lần đọc đều thấy bit 7 == 1 (Clock Halt), chắc chắn mất thời gian
    return true;
}
    
    return false;
}