#ifndef MP3_H
#define MP3_H

#include "stm32f1xx_hal.h"
#include "fatfs.h"

class Mp3 {
public:
    void init();
    void play(const char* filename);
    void setVolume(uint8_t left, uint8_t right);
    void testAudio();
    void process(); 
    bool isPlaying; 

private:
    bool waitReady();
    void writeRegister(uint8_t address, uint16_t data);
    uint16_t readRegister(uint8_t address);
    void sendDataBlock(uint8_t* data, uint16_t length);
    void reset();
    
    uint8_t audioBuffer[512]; 
    uint16_t bufferIndex;
    uint16_t dataInBuf;
};
extern Mp3 mp3;

#endif