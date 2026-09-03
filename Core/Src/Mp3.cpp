#include "Mp3.h"
#include <string.h>
#include "stm32f1xx_hal.h"

#define SCI_MODE        0x00
#define SCI_STATUS      0x01
#define SCI_BASS        0x02
#define SCI_CLOCKF      0x03
#define SCI_DECODE_TIME 0x04
#define SCI_AUDATA      0x05
#define SCI_WRAM        0x06
#define SCI_WRAMADDR    0x07
#define SCI_HDAT0       0x08
#define SCI_HDAT1       0x09
#define SCI_AIADDR      0x0A
#define SCI_VOL         0x0B
#define SCI_AICTRL0     0x0C
#define SCI_AICTRL1     0x0D
#define SCI_AICTRL2     0x0E
#define SCI_AICTRL3     0x0F


Mp3::Mp3(SpiBus* spi_bus,
        GPIO_TypeDef* pwrPort, uint16_t pwrPin,
        GPIO_TypeDef* xresetPort, uint16_t xresetPin,
        GPIO_TypeDef* dreqPort, uint16_t dreqPin,
        GPIO_TypeDef* xdcsPort, uint16_t xdcsPin,
        GPIO_TypeDef* xcsPort, uint16_t xcsPin) : 
        spi_bus(spi_bus),
        pwrPort(pwrPort), pwrPin(pwrPin),
        xresetPort(xresetPort), xresetPin(xresetPin),
        dreqPort(dreqPort), dreqPin(dreqPin),
        xdcsPort(xdcsPort), xdcsPin(xdcsPin),
        xcsPort(xcsPort), xcsPin(xcsPin),
        hasSavedState(false) {}

static FIL mp3File;

bool Mp3::waitReady() {
    uint32_t timeout = HAL_GetTick();
    while ((dreqPort->IDR & dreqPin) == 0) {
        if (HAL_GetTick() - timeout > 500) {
            return false; 
        }
    }
    return true;
}

void Mp3::writeRegister(uint8_t address, uint16_t data) {
    if (!waitReady()) return; 
    xcsPort->BRR = xcsPin;                
    
    uint8_t cmd[4];
    cmd[0] = 0x02; 
    cmd[1] = address;
    cmd[2] = (data >> 8) & 0xFF; 
    cmd[3] = data & 0xFF;        
    
    spi_bus->TransferBuffer(cmd, nullptr, 4);
    xcsPort->BSRR = xcsPin;               
}

void Mp3::reset() {
    xresetPort->BRR = xresetPin;
    HAL_Delay(20);
    xresetPort->BSRR = xresetPin;
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
    spi_bus->Init(); // Khởi tạo SPI và các chân GPIO của SPI
    GPIO_TypeDef* ports[] = {pwrPort, xresetPort, xdcsPort, xcsPort};
    uint16_t pins[] = {pwrPin, xresetPin, xdcsPin, xcsPin};
    
    for (int i = 0; i < 4; i++) {
        if (ports[i] == GPIOA) RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
        if (ports[i] == GPIOB) RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
        if (ports[i] == GPIOC) RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
        if (ports[i] == GPIOD) RCC->APB2ENR |= RCC_APB2ENR_IOPDEN;

        uint32_t pinIndex = 0;
        while (!(pins[i] & (1 << pinIndex))) pinIndex++;

        if (pinIndex < 8) {
            ports[i]->CRL &= ~(0xF << (pinIndex * 4));
            ports[i]->CRL |= (0x3 << (pinIndex * 4)); 
        } else {
            ports[i]->CRH &= ~(0xF << ((pinIndex - 8) * 4));
            ports[i]->CRH |= (0x3 << ((pinIndex - 8) * 4));
        }
    }
    
    uint32_t pinIndex = 0;
    while (!(dreqPin & (1 << pinIndex))) pinIndex++;
    if (pinIndex < 8) {
        dreqPort->CRL &= ~(0xF << (pinIndex * 4));
        dreqPort->CRL |= (0x4 << (pinIndex * 4)); // input floating
    } else {
        dreqPort->CRH &= ~(0xF << ((pinIndex - 8) * 4));
        dreqPort->CRH |= (0x4 << ((pinIndex - 8) * 4));
    }

    xcsPort->BSRR = xcsPin;
    xdcsPort->BSRR = xdcsPin;
    xresetPort->BSRR = xresetPin;
    pwrPort->BRR = pwrPin; // Keo xuong LOW de mo PNP Transistor cap nguon 3V3
    
    HAL_Delay(100); // Cho nguon on dinh truoc khi truyen SPI
    uint8_t dummy = 0xFF;
    spi_bus->Transfer(dummy);
    
    reset();
    isPlaying = false;
}

void Mp3::setVolume(uint8_t left, uint8_t right) {
    uint16_t v = (left << 8) | right;
    writeRegister(SCI_VOL, v);
}

void Mp3::sendDataBlock(uint8_t* data, uint16_t length) {
    if (!waitReady()) return; 
    xdcsPort->BRR = xdcsPin;               
    spi_bus->TransferBuffer(data, nullptr, length); 
    xdcsPort->BSRR = xdcsPin;
}

bool Mp3::play(const char* filename) {
        static FIL tempFile;
    // Kiem tra file ton tai de tranh main.cpp bao loi
    FRESULT fres = f_open(&tempFile, filename, FA_READ);
    if (fres == FR_OK) {
        f_close(&tempFile);
        
                if (queueCount < 5) {
            strncpy(queue[queueTail], filename, 31);
            queue[queueTail][31] = '\0';
            queueTail = (queueTail + 1) % 5;
            queueCount++;
        }
        return true; 
    }
    return false;
}

uint16_t Mp3::readRegister(uint8_t address) {
    if (!waitReady()) return 0;
    xcsPort->BRR = xcsPin;
    
    uint8_t tx[4] = {0x03, address, 0xFF, 0xFF};
    uint8_t rx[4] = {0, 0, 0, 0};
    spi_bus->TransferBuffer(tx, rx, 4);
    
    xcsPort->BSRR = xcsPin;
    
    return (rx[2] << 8) | rx[3];
}

void Mp3::process() {
        if (!isPlaying) {
        if (queueCount > 0) {
            FRESULT fres = f_open(&mp3File, queue[queueHead], FA_READ);
            
            if (fres == FR_OK) {
                strncpy(currentFilename, queue[queueHead], 31);
                currentFilename[31] = '\0';
                bufferIndex = 0; 
                dataInBuf = 0;
                isPlaying = true; 
            }
            
            queueHead = (queueHead + 1) % 5;
            queueCount--;
        }
        return;
    }

    int chunksSent = 0;
    while (((dreqPort->IDR & dreqPin) ? 1 : 0) == 1 && chunksSent < 16) {
        
        if (bufferIndex >= dataInBuf) {
            UINT bytesRead;
            FRESULT fres = f_read(&mp3File, audioBuffer, 512, &bytesRead);
            dataInBuf = bytesRead;
            bufferIndex = 0;

            if (bytesRead == 0) { 
                if (fres != FR_OK) {
                    // SD Card removed or read error -> Save state to resume later
                    saveState();
                    return;
                }
                
                f_close(&mp3File);
                isPlaying = false;
                
                uint8_t endFillByte = 0x00; // Always 0x00 for MP3
                
                uint8_t endFill[32];
                for(int i = 0; i < 32; i++) endFill[i] = endFillByte;
                
                for (int i = 0; i < 65; i++) { if (!waitReady()) break;
                    xdcsPort->BRR = xdcsPin;               
                    spi_bus->TransferBuffer(endFill, nullptr, 32); 
                    xdcsPort->BSRR = xdcsPin;
                }
                
                uint16_t mode = readRegister(SCI_MODE);
                mode |= 0x0008; // SM_CANCEL
                writeRegister(SCI_MODE, mode);
                
                for (int i = 0; i < 64; i++) { if (!waitReady()) break;
                    xdcsPort->BRR = xdcsPin;               
                    spi_bus->TransferBuffer(endFill, nullptr, 32); 
                    xdcsPort->BSRR = xdcsPin;
                    
                    mode = readRegister(SCI_MODE);
                    if (!(mode & 0x0008)) {
                        break; 
                    }
                }
                
                HAL_Delay(50);
                
                return; // Let next process iteration check queue
            }
        }

        uint16_t chunk = dataInBuf - bufferIndex;
        if (chunk > 32) chunk = 32;

        xdcsPort->BRR = xdcsPin;               
        spi_bus->TransferBuffer(&audioBuffer[bufferIndex], nullptr, chunk); 
        xdcsPort->BSRR = xdcsPin;
        
        bufferIndex += chunk;
        chunksSent++;
    }
}

void Mp3::testAudio() {
    writeRegister(SCI_MODE, 0x0820); 
    HAL_Delay(50);

    uint8_t sineCmd[] = {0x53, 0xEF, 0x6E, 0x44, 0x00, 0x00, 0x00, 0x00};
    
    waitReady();
    xdcsPort->BRR = xdcsPin;               
    spi_bus->TransferBuffer(sineCmd, nullptr, 8); 
    xdcsPort->BSRR = xdcsPin;

    HAL_Delay(2000); 

    uint8_t exitCmd[] = {0x45, 0x78, 0x69, 0x74, 0x00, 0x00, 0x00, 0x00};
    waitReady();
    xdcsPort->BRR = xdcsPin;
    spi_bus->TransferBuffer(exitCmd, nullptr, 8);
    xdcsPort->BSRR = xdcsPin;
    
    writeRegister(SCI_MODE, 0x0804); 
}

void Mp3::saveState() {
    if (isPlaying) {
        strncpy(savedFilename, currentFilename, 31);
        savedFilename[31] = '\0';
        savedPosition = f_tell(&mp3File);
        hasSavedState = true;
        
        f_close(&mp3File);
        isPlaying = false;
    }
}

void Mp3::resumeState() {
    if (hasSavedState) {
        FRESULT fres = f_open(&mp3File, savedFilename, FA_READ);
        if (fres == FR_OK) {
            f_lseek(&mp3File, savedPosition);
            bufferIndex = 0;
            dataInBuf = 0;
            isPlaying = true;
            
            strncpy(currentFilename, savedFilename, 31);
            currentFilename[31] = '\0';
        }
        hasSavedState = false;
    }
}



