#ifndef SPI_BUS_H
#define SPI_BUS_H

#include "stm32f1xx.h"

class SpiBus {
private:
    bool isSoftware;
    
    // Hardware SPI
    SPI_TypeDef* hspi;
    
    // Software SPI
    GPIO_TypeDef* sckPort;  uint16_t sckPin;
    GPIO_TypeDef* misoPort; uint16_t misoPin;
    GPIO_TypeDef* mosiPort; uint16_t mosiPin;
    uint32_t delay_ticks; 
    
    void delay();

public:
    // 1a. Khởi tạo Hardware SPI (Dùng cấu hình có sẵn từ CubeMX)
    SpiBus(SPI_TypeDef* spi_instance);

    // 1b. Khởi tạo Hardware SPI (Tự động cấu hình GPIO & SPI bằng Bare-metal)
    SpiBus(SPI_TypeDef* spi_instance, 
           GPIO_TypeDef* sckPort, uint16_t sckPin,
           GPIO_TypeDef* misoPort, uint16_t misoPin,
           GPIO_TypeDef* mosiPort, uint16_t mosiPin);
    
    // 2. Khởi tạo Software SPI
    SpiBus(GPIO_TypeDef* sckPort, uint16_t sckPin,
           GPIO_TypeDef* misoPort, uint16_t misoPin,
           GPIO_TypeDef* mosiPort, uint16_t mosiPin);
                
    void Init(); // Dùng chung cho cả SPI Cứng và SPI Mềm

    uint8_t Transfer(uint8_t data);
    void TransferBuffer(const uint8_t* tx_data, uint8_t* rx_data, uint16_t size);
    
    void SetSpeed(uint32_t speed_hz);
    void SetPrescaler(uint32_t prescaler);
};

#endif // SPI_BUS_H
