#ifndef MP3_H
#define MP3_H

#include "stm32f1xx_hal.h"
#include "fatfs.h"

class Mp3 {
public:
    Mp3(SPI_HandleTypeDef* hspi,
        GPIO_TypeDef* xresetPort, uint16_t xresetPin,
        GPIO_TypeDef* dreqPort, uint16_t dreqPin,
        GPIO_TypeDef* xdcsPort, uint16_t xdcsPin,
        GPIO_TypeDef* xcsPort, uint16_t xcsPin);

    void init();
    bool play(const char* filename);
    void setVolume(uint8_t left, uint8_t right);
    void testAudio();
    void process(); 
    bool isPlaying; 

private:
    SPI_HandleTypeDef* hspi;
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