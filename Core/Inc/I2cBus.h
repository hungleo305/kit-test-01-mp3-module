#ifndef I2C_BUS_H
#define I2C_BUS_H

#include "stm32f1xx.h"

class I2cBus {
private:
    bool isSoftware;
    
    // Hardware I2C
    I2C_TypeDef* hi2c;
    
    // Software I2C
    GPIO_TypeDef* sclPort; uint16_t sclPin;
    GPIO_TypeDef* sdaPort; uint16_t sdaPin;
    
    void delay();
    void startCond();
    void stopCond();
    bool sendByte(uint8_t data);
    uint8_t readByte(bool ack);

public:
    // 1a. Khởi tạo Hardware I2C (Dùng cấu hình có sẵn từ CubeMX)
    I2cBus(I2C_TypeDef* i2c_instance);
    
    // 1b. Khởi tạo Hardware I2C (Tự động cấu hình GPIO & I2C bằng Bare-metal)
    I2cBus(I2C_TypeDef* i2c_instance, 
           GPIO_TypeDef* sclPort, uint16_t sclPin,
           GPIO_TypeDef* sdaPort, uint16_t sdaPin);
    
    // 2. Khởi tạo Software I2C
    I2cBus(GPIO_TypeDef* sclPort, uint16_t sclPin,
           GPIO_TypeDef* sdaPort, uint16_t sdaPin);
    
    void Init();
    
    // Ghi 1 byte vào một thanh ghi của thiết bị
    bool WriteMem(uint8_t devAddr, uint8_t memAddr, uint8_t data);
    
    // Đọc 1 byte từ một thanh ghi của thiết bị
    uint8_t ReadMem(uint8_t devAddr, uint8_t memAddr);
};

#endif // I2C_BUS_H
