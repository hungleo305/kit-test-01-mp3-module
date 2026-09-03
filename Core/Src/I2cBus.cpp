#include "I2cBus.h"

// Hardware I2C (CubeMX)
I2cBus::I2cBus(I2C_TypeDef* i2c_instance) {
    this->hi2c = i2c_instance;
    this->isSoftware = false;
    this->sclPort = nullptr;
    this->sdaPort = nullptr;
}

// Hardware I2C (Bare-metal)
I2cBus::I2cBus(I2C_TypeDef* i2c_instance, GPIO_TypeDef* sclPort, uint16_t sclPin, GPIO_TypeDef* sdaPort, uint16_t sdaPin) {
    this->hi2c = i2c_instance;
    this->sclPort = sclPort;
    this->sclPin = sclPin;
    this->sdaPort = sdaPort;
    this->sdaPin = sdaPin;
    this->isSoftware = false;
}

// Software I2C
I2cBus::I2cBus(GPIO_TypeDef* sclPort, uint16_t sclPin, GPIO_TypeDef* sdaPort, uint16_t sdaPin) {
    this->hi2c = nullptr;
    this->sclPort = sclPort;
    this->sclPin = sclPin;
    this->sdaPort = sdaPort;
    this->sdaPin = sdaPin;
    this->isSoftware = true;
}

void I2cBus::delay() {
    for (volatile int i = 0; i < 72; i++) {} // ~5us delay for 100kHz standard mode (STM32F103 72MHz)
}

void I2cBus::Init() {
    if (sclPort != nullptr && sdaPort != nullptr) {
        // Bật Clock cho các port
        if (sclPort == GPIOA || sdaPort == GPIOA) RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
        if (sclPort == GPIOB || sdaPort == GPIOB) RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
        if (sclPort == GPIOC || sdaPort == GPIOC) RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
        if (sclPort == GPIOD || sdaPort == GPIOD) RCC->APB2ENR |= RCC_APB2ENR_IOPDEN;
        
        RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;
        
        GPIO_TypeDef* ports[] = {sclPort, sdaPort};
        uint16_t pins[] = {sclPin, sdaPin};
        
        for (int i = 0; i < 2; i++) {
            uint32_t pinIndex = 0;
            while (!(pins[i] & (1 << pinIndex))) pinIndex++;
            
            if (pinIndex < 8) {
                ports[i]->CRL &= ~(0xF << (pinIndex * 4));
                if (isSoftware) {
                    ports[i]->CRL |= (0x6 << (pinIndex * 4)); // General Purpose Open Drain 50MHz
                } else {
                    ports[i]->CRL |= (0xF << (pinIndex * 4)); // Alternate Function Open Drain 50MHz
                }
            } else {
                ports[i]->CRH &= ~(0xF << ((pinIndex - 8) * 4));
                if (isSoftware) {
                    ports[i]->CRH |= (0x6 << ((pinIndex - 8) * 4));
                } else {
                    ports[i]->CRH |= (0xF << ((pinIndex - 8) * 4));
                }
            }
        }
        
        // Mặc định kéo chân lên mức cao (I2C idle state)
        sclPort->BSRR = sclPin;
        sdaPort->BSRR = sdaPin;
    }
    
    if (!isSoftware && hi2c != nullptr && sclPort != nullptr) {
        // Cấu hình Hardware I2C (Bare-metal)
        if (hi2c == I2C1) RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;
        else if (hi2c == I2C2) RCC->APB1ENR |= RCC_APB1ENR_I2C2EN;
        
        hi2c->CR1 |= I2C_CR1_SWRST;
        hi2c->CR1 &= ~I2C_CR1_SWRST;
        
        hi2c->CR2 = 36; // Cấu hình tần số ngoại vi (36MHz cho APB1 trên STM32F103)
        hi2c->CCR = 180; // Standard mode 100kHz
        hi2c->TRISE = 37;
        hi2c->CR1 |= I2C_CR1_PE;
    }
}

void I2cBus::resetBus() {
    if (!isSoftware && hi2c != nullptr) {
        hi2c->CR1 |= I2C_CR1_SWRST;
        hi2c->CR1 &= ~I2C_CR1_SWRST;
        hi2c->CR2 = 36;
        hi2c->CCR = 180;
        hi2c->TRISE = 37;
        hi2c->CR1 |= I2C_CR1_PE;
    }
}

// ==== SOFTWARE I2C IMPLEMENTATION ====

void I2cBus::startCond() {
    sdaPort->BSRR = sdaPin;
    sclPort->BSRR = sclPin;
    delay();
    sdaPort->BRR = sdaPin;
    delay();
    sclPort->BRR = sclPin;
    delay();
}

void I2cBus::stopCond() {
    sdaPort->BRR = sdaPin;
    sclPort->BRR = sclPin;
    delay();
    sclPort->BSRR = sclPin;
    delay();
    sdaPort->BSRR = sdaPin;
    delay();
}

bool I2cBus::sendByte(uint8_t data) {
    for (int i = 0; i < 8; i++) {
        if (data & 0x80) sdaPort->BSRR = sdaPin;
        else sdaPort->BRR = sdaPin;
        delay();
        sclPort->BSRR = sclPin;
        delay();
        sclPort->BRR = sclPin;
        data <<= 1;
    }
    // Read ACK
    sdaPort->BSRR = sdaPin; // Release SDA
    delay();
    sclPort->BSRR = sclPin;
    delay();
    bool ack = !(sdaPort->IDR & sdaPin); // 0 is ACK, 1 is NACK
    sclPort->BRR = sclPin;
    return ack;
}

uint8_t I2cBus::readByte(bool ack) {
    uint8_t data = 0;
    sdaPort->BSRR = sdaPin; // Release SDA
    for (int i = 0; i < 8; i++) {
        data <<= 1;
        sclPort->BSRR = sclPin;
        delay();
        if (sdaPort->IDR & sdaPin) data |= 0x01;
        sclPort->BRR = sclPin;
        delay();
    }
    // Send ACK/NACK
    if (ack) sdaPort->BRR = sdaPin; // 0 for ACK
    else sdaPort->BSRR = sdaPin;    // 1 for NACK
    delay();
    sclPort->BSRR = sclPin;
    delay();
    sclPort->BRR = sclPin;
    return data;
}

bool I2cBus::WriteMem(uint8_t devAddr, uint8_t memAddr, uint8_t data) {
    if (isSoftware) {
        startCond();
        if (!sendByte(devAddr)) { stopCond(); return false; } // devAddr has R/W bit = 0
        if (!sendByte(memAddr)) { stopCond(); return false; }
        if (!sendByte(data)) { stopCond(); return false; }
        stopCond();
        return true;
    } else {
        uint32_t t = 50000;
        while ((hi2c->SR2 & I2C_SR2_BUSY) && --t); if(!t) { resetBus(); return false; }

        hi2c->CR1 |= I2C_CR1_START;
        t = 50000; while (!(hi2c->SR1 & I2C_SR1_SB) && --t); if(!t) { resetBus(); return false; }

        hi2c->DR = devAddr;
        t = 50000; while (!(hi2c->SR1 & I2C_SR1_ADDR) && --t); if(!t) { resetBus(); return false; }
        (void)hi2c->SR2;

        hi2c->DR = memAddr;
        t = 50000; while (!(hi2c->SR1 & I2C_SR1_TXE) && --t); if(!t) { resetBus(); return false; }

        hi2c->DR = data;
        t = 50000; while (!(hi2c->SR1 & I2C_SR1_BTF) && --t); if(!t) { resetBus(); return false; }

        hi2c->CR1 |= I2C_CR1_STOP;
        return true;
    }
}

uint8_t I2cBus::ReadMem(uint8_t devAddr, uint8_t memAddr) {
    if (isSoftware) {
        startCond();
        if (!sendByte(devAddr)) { stopCond(); return 0; }
        if (!sendByte(memAddr)) { stopCond(); return 0; }
        
        startCond(); // Repeated start
        if (!sendByte(devAddr | 0x01)) { stopCond(); return 0; } // devAddr with Read bit
        uint8_t data = readByte(false); // NACK on last byte
        stopCond();
        return data;
    } else {
        uint8_t data = 0;
        uint32_t t = 50000;

        while ((hi2c->SR2 & I2C_SR2_BUSY) && --t); if(!t) { resetBus(); return 0; }

        hi2c->CR1 |= I2C_CR1_START;                 
        t = 50000; while (!(hi2c->SR1 & I2C_SR1_SB) && --t); if(!t) { resetBus(); return 0; }

        hi2c->DR = devAddr;                            
        t = 50000; while (!(hi2c->SR1 & I2C_SR1_ADDR) && --t); if(!t) { resetBus(); return 0; }
        (void)hi2c->SR2;                            

        hi2c->DR = memAddr;                            
        t = 50000; while (!(hi2c->SR1 & I2C_SR1_TXE) && --t); if(!t) { resetBus(); return 0; }

        hi2c->CR1 |= I2C_CR1_START;                 
        t = 50000; while (!(hi2c->SR1 & I2C_SR1_SB) && --t); if(!t) { resetBus(); return 0; }

        hi2c->DR = devAddr | 0x01;                            
        t = 50000; while (!(hi2c->SR1 & I2C_SR1_ADDR) && --t); if(!t) { resetBus(); return 0; }

        hi2c->CR1 &= ~I2C_CR1_ACK;                  
        (void)hi2c->SR2;                         

        hi2c->CR1 |= I2C_CR1_STOP;                  
        t = 50000; while (!(hi2c->SR1 & I2C_SR1_RXNE) && --t); if(!t) { resetBus(); return 0; }

        data = hi2c->DR;                            
        return data;
    }
}

bool I2cBus::WriteByte(uint8_t devAddr, uint8_t data) {
    if (isSoftware) {
        startCond();
        if (!sendByte(devAddr)) { stopCond(); return false; }
        if (!sendByte(data)) { stopCond(); return false; }
        stopCond();
        return true;
    } else {
        uint32_t t = 50000;
        while ((hi2c->SR2 & I2C_SR2_BUSY) && --t); if(!t) { resetBus(); return false; }

        hi2c->CR1 |= I2C_CR1_START;
        t = 50000; while (!(hi2c->SR1 & I2C_SR1_SB) && --t); if(!t) { resetBus(); return false; }

        hi2c->DR = devAddr;
        t = 50000; while (!(hi2c->SR1 & I2C_SR1_ADDR) && --t); if(!t) { resetBus(); return false; }
        (void)hi2c->SR2;

        hi2c->DR = data;
        t = 50000; while (!(hi2c->SR1 & I2C_SR1_TXE) && --t); if(!t) { resetBus(); return false; }
        t = 50000; while (!(hi2c->SR1 & I2C_SR1_BTF) && --t); if(!t) { resetBus(); return false; }

        hi2c->CR1 |= I2C_CR1_STOP;
        return true;
    }
}

uint8_t I2cBus::ReadByteDirect(uint8_t devAddr) {
    if (isSoftware) {
        startCond();
        if (!sendByte(devAddr | 0x01)) { stopCond(); return 0; }
        uint8_t data = readByte(false); // NACK on last byte
        stopCond();
        return data;
    } else {
        uint8_t data = 0;
        uint32_t t = 50000;
        while ((hi2c->SR2 & I2C_SR2_BUSY) && --t); if(!t) { resetBus(); return 0; }

        hi2c->CR1 |= I2C_CR1_START;
        t = 50000; while (!(hi2c->SR1 & I2C_SR1_SB) && --t); if(!t) { resetBus(); return 0; }

        hi2c->DR = devAddr | 0x01;
        t = 50000; while (!(hi2c->SR1 & I2C_SR1_ADDR) && --t); if(!t) { resetBus(); return 0; }
        
        hi2c->CR1 &= ~I2C_CR1_ACK;
        (void)hi2c->SR2;
        hi2c->CR1 |= I2C_CR1_STOP;

        t = 50000; while (!(hi2c->SR1 & I2C_SR1_RXNE) && --t); if(t) data = hi2c->DR; else resetBus();

        return data;
    }
}
