#ifndef __SDCARD_H
#define __SDCARD_H

#include "stm32f1xx.h"
#include "SpiBus.h"

#ifdef __cplusplus
class SdCard {
private:
    GPIO_TypeDef* cdPort;
    uint16_t cdPin;
    GPIO_TypeDef* pwrPort;
    uint16_t pwrPin;
    SpiBus* spi_bus;
    GPIO_TypeDef *cs_port;
    uint16_t cs_pin;
    uint8_t stat;
    uint8_t cardType;

    void setSpiSpeedSlow();
    void setSpiSpeedFast();

    void csLow();
    void csHigh();

    uint8_t waitReady();
    uint8_t sendCmd(uint8_t cmd, uint32_t arg);

public:
    static SdCard* instance;
    SdCard(SpiBus* spi_bus, GPIO_TypeDef* csPort, uint16_t csPin, GPIO_TypeDef* cdPort, uint16_t cdPin, GPIO_TypeDef* pwrPort, uint16_t pwrPin);
    
    // Cấu hình chân GPIO ban đầu cho các chân CS, CD (Card Detect) và PWR (Power)
    void init();
    
    // Kiểm tra trạng thái chân CD xem thẻ đang được cắm (true) hay rút ra (false)
    bool checkPhysicalPresence();
    
    // Đóng cầu dao cấp nguồn cứng cho thẻ SD (thông qua chân PWR)
    void powerOn();
    
    // Ngắt nguồn cứng của thẻ SD (để reset hoặc bảo vệ thẻ khi rút)
    void powerOff();
    
    // Khởi tạo thẻ theo chuẩn SPI (chạy xung clock chậm, gửi CMD0 để reset, kiểm tra type SDHC/SDXC...)
    uint8_t initialize();
    
    // Đọc trạng thái hiện tại của thẻ (Sẵn sàng, Lỗi, Không có thẻ...)
    uint8_t status();
    
    // Đọc số lượng `count` sector (thường mỗi sector = 512 byte) từ vị trí `sector` vào mảng `buff`
    uint8_t read(uint8_t *buff, uint32_t sector, uint32_t count);
    
    // Ghi `count` sector từ mảng `buff` vào thẻ SD tại vị trí `sector`
    uint8_t write(const uint8_t *buff, uint32_t sector, uint32_t count);
    
    // Gửi các lệnh đặc biệt (Input/Output Control) ví dụ: đọc dung lượng thẻ, lấy thông số block...
    uint8_t ioctl(uint8_t cmd, void *buff);
};
#endif

#ifdef __cplusplus
extern "C" {
#endif

// ====== Các hàm bọc (Wrapper) dành riêng cho FatFs ======
// FatFs là thư viện C thuần, nên nó không gọi được các hàm trong class (C++) bên trên.
// Vì vậy ta phải định nghĩa các hàm C-style gọi ngược lại các phương thức của SdCard::instance.
uint8_t sdCard_initialize(void);
uint8_t sdCard_status(void);
uint8_t sdCard_read(uint8_t *buff, uint32_t sector, uint32_t count);
uint8_t sdCard_write(const uint8_t *buff, uint32_t sector, uint32_t count);
uint8_t sdCard_ioctl(uint8_t cmd, void *buff);

#ifdef __cplusplus
}
#endif

#endif


