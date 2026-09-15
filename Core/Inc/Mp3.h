#ifndef MP3_H
#define MP3_H

#include "SpiBus.h"
#include "stm32f1xx_hal.h"
#include "ff.h"

class Mp3 {
public:
    Mp3(SpiBus* spi_bus,
        GPIO_TypeDef* pwrPort,    uint16_t pwrPin,
        GPIO_TypeDef* xresetPort, uint16_t xresetPin,
        GPIO_TypeDef* dreqPort,   uint16_t dreqPin,
        GPIO_TypeDef* xdcsPort,   uint16_t xdcsPin,
        GPIO_TypeDef* xcsPort,    uint16_t xcsPin);

    // Khởi tạo và điều khiển
    void init();
    void reset();
    void process();
    void testAudio();

    // Điều khiển âm thanh
    void  setVolume(uint8_t left, uint8_t right);
    bool  setPlaySpeed(float speedMultiplier);   // Trả về true nếu thành công
    float getPlaySpeed() const { return playSpeed; }

    // Hàng đợi file
    bool play(const char* filename);

    // Lưu/khôi phục trạng thái khi rút thẻ
    void saveState();
    void resumeState();

private:
    // Phần cứng
    SpiBus* spi_bus;
    GPIO_TypeDef *pwrPort, *xresetPort, *dreqPort, *xdcsPort, *xcsPort;
    uint16_t pwrPin, xresetPin, dreqPin, xdcsPin, xcsPin;

    // Hàng đợi
    char     queue[5][32];
    volatile uint8_t queueHead, queueTail, queueCount;

    // Trạng thái
    bool  isPlaying;
    bool  hasSavedState;
    float playSpeed;

    char  currentFilename[32];
    char  savedFilename[32];
    DWORD savedPosition;

    // Bộ đệm đọc SD
    uint8_t  audioBuffer[512];
    uint16_t bufferIndex;
    uint16_t dataInBuf;

    // Hàm nội bộ
    bool     waitReady();
    void     writeRegister(uint8_t addr, uint16_t data);
    uint16_t readRegister(uint8_t addr);

    // Chuyển đổi hệ số tốc độ -> giá trị SCI_CLOCKF
    uint16_t speedToClockf(float speed);
};

#endif