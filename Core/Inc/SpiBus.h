#ifndef SPI_BUS_H
#define SPI_BUS_H

#include "stm32f1xx.h"

class SpiBus {
private:
    bool isSoftware;
    bool use_dma_flag = true;
    
    // Hardware SPI
    SPI_TypeDef* hspi;
    
    // Software SPI
    GPIO_TypeDef* sckPort;  uint16_t sckPin;
    GPIO_TypeDef* misoPort; uint16_t misoPin;
    GPIO_TypeDef* mosiPort; uint16_t mosiPin;
    uint32_t delay_ticks; 
    
    // DMA Channels
    DMA_Channel_TypeDef* dma_rx;
    DMA_Channel_TypeDef* dma_tx;
    
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
                
    // Khởi tạo SPI (Tự động cấu hình GPIO nếu dùng Bare-metal, hoặc chỉ reset nếu dùng cấu hình CubeMX)
    void Init(); 

    // Gửi đi 1 byte từ Master sang Slave, đồng thời đọc về 1 byte từ Slave (Full-duplex)
    uint8_t Transfer(uint8_t data);
    
    // Gửi và nhận một chuỗi byte (mảng) sử dụng cơ chế Polling (CPU đứng chờ cho đến khi gửi xong)
    void TransferBuffer(const uint8_t* tx_data, uint8_t* rx_data, uint16_t size);
    
    // Gửi và nhận chuỗi byte sử dụng DMA (truy cập bộ nhớ trực tiếp, giải phóng CPU làm việc khác)
    void TransferDMA(const uint8_t* tx_data, uint8_t* rx_data, uint16_t size);
    
    // Cho phép (true) hoặc vô hiệu hóa (false) chức năng truyền DMA của thư viện
    void EnableDMA(bool enable) { use_dma_flag = enable; }
    
    // Cài đặt tốc độ truyền (tần số Clock) cho SPI (Tính theo đơn vị Hertz, VD: 400000 = 400kHz)
    void SetSpeed(uint32_t speed_hz);
    
    // Cài đặt bộ chia tần số cứng (Prescaler) cho khối SPI (VD: SPI_CR1_BR_1 để chia 8)
    void SetPrescaler(uint32_t prescaler);
};

#endif // SPI_BUS_H
