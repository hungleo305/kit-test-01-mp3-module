#include "Mp3.h"
#include "Buzzer.h"

extern SPI_HandleTypeDef hspi2; 

#define VS_XRESET_HIGH() HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET)
#define VS_XRESET_LOW()  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET)

#define VS_XCS_HIGH()    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_SET)   // XCS  PC3
#define VS_XCS_LOW()     HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_RESET) 

#define VS_XDCS_HIGH()   HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET)  //XDCS  PB12
#define VS_XDCS_LOW()    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET)

#define VS_DREQ_READ()   HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1)

#define SCI_MODE        0x00
#define SCI_VOL         0x0B
#define SCI_CLOCKF 0x03
Mp3 mp3;

static FIL mp3File;

bool Mp3::waitReady() {
    uint32_t timeout = HAL_GetTick();
    while (VS_DREQ_READ() == 0) {
        if (HAL_GetTick() - timeout > 500) {
            extern Buzzer buzzer; 
            buzzer.beep(10, 50, 50); 
            return false; 
        }
    }
    return true;
}

void Mp3::writeRegister(uint8_t address, uint16_t data) {
    if (!waitReady()) return; 
    VS_XCS_LOW();                
    
    uint8_t cmd[4];
    cmd[0] = 0x02; 
    cmd[1] = address;
    cmd[2] = (data >> 8) & 0xFF; 
    cmd[3] = data & 0xFF;        
    
    HAL_SPI_Transmit(&hspi2, cmd, 4, 100);
    VS_XCS_HIGH();               
}

void Mp3::reset() {
    VS_XRESET_LOW();
    HAL_Delay(20);
    VS_XRESET_HIGH();
    HAL_Delay(20);
    
    writeRegister(SCI_MODE, 0x0804);
    HAL_Delay(20);
    waitReady();
    
    writeRegister(SCI_CLOCKF, 0x9800); 
    HAL_Delay(20);
    waitReady();
    
    setVolume(50, 50); 
}

void Mp3::init() {
    VS_XCS_HIGH();
    VS_XDCS_HIGH();
    VS_XRESET_HIGH();
    
    HAL_Delay(50);
    uint8_t dummy = 0xFF;
    HAL_SPI_Transmit(&hspi2, &dummy, 1, 100);
    
    reset();
    isPlaying = false;
}

void Mp3::setVolume(uint8_t left, uint8_t right) {
    uint16_t v = (left << 8) | right;
    writeRegister(SCI_VOL, v);
}

void Mp3::sendDataBlock(uint8_t* data, uint16_t length) {
    if (!waitReady()) return; 
    VS_XDCS_LOW();               
    HAL_SPI_Transmit(&hspi2, data, length, 100); 
    VS_XDCS_HIGH();
}

void Mp3::play(const char* filename) {
    if (isPlaying) {
        f_close(&mp3File);
        isPlaying = false;
    }
    
    FRESULT fres = f_open(&mp3File, filename, FA_READ);
    if (fres == FR_OK) {
        bufferIndex = 0; 
        dataInBuf = 0;
        isPlaying = true; 
    } else {
        extern Buzzer buzzer;
        buzzer.beep(3, 100, 100); 
    }
}

void Mp3::process() {
    if (!isPlaying) return;

    int chunksSent = 0;
    while (VS_DREQ_READ() == 1 && chunksSent < 16) {
        
        if (bufferIndex >= dataInBuf) {
            UINT bytesRead;
            f_read(&mp3File, audioBuffer, 512, &bytesRead);
            dataInBuf = bytesRead;
            bufferIndex = 0;

            if (bytesRead == 0) { 
                f_close(&mp3File);
                isPlaying = false;
                
                uint8_t endFill[32] = {0};
                for (int i = 0; i < 64; i++) {
                    waitReady();
                    VS_XDCS_LOW();               
                    HAL_SPI_Transmit(&hspi2, endFill, 32, 100); 
                    VS_XDCS_HIGH();
                }
                return;
            }
        }

        uint16_t chunk = dataInBuf - bufferIndex;
        if (chunk > 32) chunk = 32;

        VS_XDCS_LOW();               
        HAL_SPI_Transmit(&hspi2, &audioBuffer[bufferIndex], chunk, 100); 
        VS_XDCS_HIGH();
        
        bufferIndex += chunk;
        chunksSent++;
    }
}

void Mp3::testAudio() {
    writeRegister(SCI_MODE, 0x0820); 
    HAL_Delay(50);

    uint8_t sineCmd[] = {0x53, 0xEF, 0x6E, 0x44, 0x00, 0x00, 0x00, 0x00};
    
    waitReady();
    VS_XDCS_LOW();               
    HAL_SPI_Transmit(&hspi2, sineCmd, 8, 100); 
    VS_XDCS_HIGH();

    HAL_Delay(2000); 

    uint8_t exitCmd[] = {0x45, 0x78, 0x69, 0x74, 0x00, 0x00, 0x00, 0x00};
    waitReady();
    VS_XDCS_LOW();
    HAL_SPI_Transmit(&hspi2, exitCmd, 8, 100);
    VS_XDCS_HIGH();
    
    writeRegister(SCI_MODE, 0x0804); 
}