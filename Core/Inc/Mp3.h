#ifndef MP3_H
#define MP3_H

#include "stm32f1xx_hal.h"
#include "fatfs.h"
#include "SpiBus.h"

class Mp3 {
public:
    Mp3(SpiBus* spi_bus,
        GPIO_TypeDef* pwrPort, uint16_t pwrPin,
        GPIO_TypeDef* xresetPort, uint16_t xresetPin,
        GPIO_TypeDef* dreqPort, uint16_t dreqPin,
        GPIO_TypeDef* xdcsPort, uint16_t xdcsPin,
        GPIO_TypeDef* xcsPort, uint16_t xcsPin);

    // Khởi tạo module VS1053 (cấp nguồn, reset cứng, reset mềm, cấu hình volume mặc định)
    void init();
    
    // Yêu cầu phát một file MP3 từ thẻ nhớ.
    // Trả về true nếu mở file thành công, false nếu file không tồn tại hoặc lỗi thẻ.
    bool play(const char* filename);
    
    // Điều chỉnh âm lượng (Volume). Giá trị từ 0 (to nhất) đến 254 (nhỏ nhất / tắt tiếng).
    void setVolume(uint8_t left, uint8_t right);
    
    // Hàm test nhanh bằng cách xuất ra một âm thanh bíp phần cứng (không cần thẻ nhớ).
    void testAudio();
    
    // Hàm cốt lõi: Bơm dữ liệu âm thanh từ thẻ nhớ vào chip VS1053.
    // Bắt buộc phải được gọi liên tục trong vòng lặp while(1) để duy trì bài nhạc không bị đứt quãng.
    void process(); 
    
    // Lưu lại trạng thái bài hát đang phát dở (Tên file và Vị trí byte đang đọc) trước khi bị ngắt quãng.
    void saveState();
    
    // Khôi phục và phát tiếp bài hát từ đúng vị trí đã lưu trước đó.
    void resumeState();
    
    // Cờ trạng thái: Bằng true nếu đang có bài hát được phát, false nếu đã phát xong hoặc đang dừng.
    bool isPlaying; 

private:
    char currentFilename[32];
    char savedFilename[32];
    uint32_t savedPosition;
    bool hasSavedState;
    SpiBus* spi_bus;
    GPIO_TypeDef* pwrPort;    uint16_t pwrPin;
    GPIO_TypeDef* xresetPort; uint16_t xresetPin;
    GPIO_TypeDef* dreqPort;   uint16_t dreqPin;
    GPIO_TypeDef* xdcsPort;   uint16_t xdcsPin;
    GPIO_TypeDef* xcsPort;    uint16_t xcsPin;

    bool waitReady();
    void writeRegister(uint8_t address, uint16_t data);
    uint16_t readRegister(uint8_t address);
    void sendDataBlock(uint8_t* data, uint16_t length);
    void reset();
    
    uint8_t audioBuffer[512]; 
    uint16_t bufferIndex;
    uint16_t dataInBuf;

    // Queue for audio files
    char queue[5][32];
    uint8_t queueHead;
    uint8_t queueTail;
    uint8_t queueCount;
};

#endif