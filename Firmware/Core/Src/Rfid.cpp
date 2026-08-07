#include "Rfid.h"

#define CS_LOW()    do { \
                        while (SPI1->SR & SPI_SR_BSY); \
                        AFIO->MAPR |= AFIO_MAPR_SPI1_REMAP; \
                        GPIOA->BRR = (1 << 15); \
                    } while(0)
#define CS_HIGH()   do { \
                        while (SPI1->SR & SPI_SR_BSY); \
                        GPIOA->BSRR = (1 << 15); \
                    } while(0)

#define RST_LOW()   (GPIOC->BRR = (1 << 14))
#define RST_HIGH()  (GPIOC->BSRR = (1 << 14))

void Rfid::init() {
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN | RCC_APB2ENR_IOPCEN | RCC_APB2ENR_AFIOEN | RCC_APB2ENR_SPI1EN;
    
    AFIO->MAPR = (AFIO->MAPR & ~AFIO_MAPR_SWJ_CFG_Msk) | AFIO_MAPR_SWJ_CFG_JTAGDISABLE;
    AFIO->MAPR |= AFIO_MAPR_SPI1_REMAP; 
    
    GPIOA->CRH &= ~(0xF << 28); GPIOA->CRH |=  (0x3 << 28);
    GPIOC->CRH &= ~(0xF << 24); GPIOC->CRH |=  (0x3 << 24); 
    GPIOB->CRL &= ~(0xF << 12 | 0xF << 20); GPIOB->CRL |=  (0xB << 12 | 0xB << 20); 
    GPIOB->CRL &= ~(0xF << 16); GPIOB->CRL |=  (0x4 << 16); 
    GPIOB->CRH &= ~(0xF << 4);  GPIOB->CRH |=  (0x8 << 4); GPIOB->ODR |= (1 << 9); 
    
    SPI1->CR1 = 0x0000;
    SPI1->CR1 |= SPI_CR1_MSTR | SPI_CR1_BR_1 | SPI_CR1_BR_0 | SPI_CR1_SSM | SPI_CR1_SSI; 
    SPI1->CR1 |= SPI_CR1_SPE; 
    
    CS_HIGH();
    RST_HIGH();
    
    resetPCD();
    writeRegister(MFRC522_REG_T_MODE, 0x8D);
    writeRegister(MFRC522_REG_T_PRESCALER, 0x3E);
    writeRegister(MFRC522_REG_T_RELOAD_L, 30);
    writeRegister(MFRC522_REG_T_RELOAD_H, 0);
    writeRegister(MFRC522_REG_TX_AUTO, 0x40);
    writeRegister(MFRC522_REG_MODE, 0x3D);
    antennaOn();
}

void Rfid::resetPCD() {
    writeRegister(MFRC522_REG_COMMAND, PCD_SOFTRESET); 
    for (volatile uint32_t i = 0; i < 10000; i++); 
}

void Rfid::antennaOn() {
    uint8_t temp = readRegister(MFRC522_REG_TX_CONTROL);
    if ((temp & 0x03) != 0x03) {
        setBitMask(MFRC522_REG_TX_CONTROL, 0x03);
    }
}

bool Rfid::checkCard() {
    uint8_t reqMode = PICC_REQIDL;
    uint8_t backData[2];
    uint32_t backLen;
    writeRegister(MFRC522_REG_BIT_FRAMING, 0x07);
    uint8_t status = toCard(PCD_TRANSCEIVE, &reqMode, 1, backData, &backLen);
    return (status == STATUS_OK && backLen == 0x10);
}

bool Rfid::readCardSerial(uint8_t* serial) {
    uint8_t status;
    uint32_t unLen;
    serial[0] = PICC_ANTICOLL;
    serial[1] = 0x20;
    
    clearBitMask(MFRC522_REG_STATUS2, 0x08);
    writeRegister(MFRC522_REG_BIT_FRAMING, 0x00);
    clearBitMask(MFRC522_REG_COLL, 0x80);
    
    status = toCard(PCD_TRANSCEIVE, serial, 2, serial, &unLen);
    
    if (status == STATUS_OK) {
        uint8_t check = 0;
        for (int i = 0; i < 4; i++) {
            check ^= serial[i];
        }
        if (check != serial[4]) return false; 
        return true;
    }
    return false;
}

uint8_t Rfid::toCard(uint8_t command, uint8_t* sendData, uint8_t sendLen, uint8_t* backData, uint32_t* backLen) {
    uint8_t status = STATUS_ERROR;
    uint8_t irqEn = 0x77;
    uint8_t waitIRq = 0x30;
    uint8_t lastBits;
    uint8_t n;
    uint32_t i;

    writeRegister(MFRC522_REG_COM_I_EN, irqEn | 0x80);
    clearBitMask(MFRC522_REG_COM_IRQ, 0x80);
    setBitMask(MFRC522_REG_FIFO_LEVEL, 0x80);
    writeRegister(MFRC522_REG_COMMAND, PCD_IDLE);

    for (i = 0; i < sendLen; i++) {
        writeRegister(MFRC522_REG_FIFO_DATA, sendData[i]);
    }
    writeRegister(MFRC522_REG_COMMAND, command);
    if (command == PCD_TRANSCEIVE) {
        setBitMask(MFRC522_REG_BIT_FRAMING, 0x80);
    }

    i = 2000; 
    do {
        n = readRegister(MFRC522_REG_COM_IRQ);
        i--;
    } while ((i != 0) && !(n & 0x01) && !(n & waitIRq));

    clearBitMask(MFRC522_REG_BIT_FRAMING, 0x80);
    if (i != 0) {
        if (!(readRegister(MFRC522_REG_ERROR) & 0x1B)) {
            status = STATUS_OK;
            if (n & irqEn & 0x01) status = STATUS_ERROR;
            if (command == PCD_TRANSCEIVE) {
                n = readRegister(MFRC522_REG_FIFO_LEVEL);
                lastBits = readRegister(MFRC522_REG_CONTROL) & 0x07;
                if (lastBits) *backLen = (n - 1) * 8 + lastBits;
                else *backLen = n * 8;
                if (n == 0) n = 1;
                if (n > 16) n = 16;
                for (i = 0; i < n; i++) {
                    backData[i] = readRegister(MFRC522_REG_FIFO_DATA);
                }
            }
        } else { status = STATUS_ERROR; }
    }
    return status;
}

uint8_t Rfid::spiTransfer(uint8_t data) {
    uint32_t timeout = 500;
    while (!(SPI1->SR & SPI_SR_TXE) && --timeout);
    SPI1->DR = data;
    
    timeout = 500;
    while (!(SPI1->SR & SPI_SR_RXNE) && --timeout);
    return SPI1->DR;
}

void Rfid::writeRegister(uint8_t addr, uint8_t val) {
    CS_LOW();
    spiTransfer((addr << 1) & 0x7E);
    spiTransfer(val);
    CS_HIGH();
}

uint8_t Rfid::readRegister(uint8_t addr) {
    CS_LOW();
    spiTransfer(((addr << 1) & 0x7E) | 0x80);
    uint8_t val = spiTransfer(0x00);
    CS_HIGH();
    return val;
}

void Rfid::setBitMask(uint8_t reg, uint8_t mask) {
    uint8_t tmp = readRegister(reg);
    writeRegister(reg, tmp | mask);
}

void Rfid::clearBitMask(uint8_t reg, uint8_t mask) {
    uint8_t tmp = readRegister(reg);
    writeRegister(reg, tmp & (~mask));
}

//bao mat

void Rfid::calculateCRC(uint8_t *data, uint8_t length, uint8_t *result) {
    writeRegister(MFRC522_REG_COMMAND, PCD_IDLE);
    clearBitMask(MFRC522_REG_DIV_IRQ, 0x04);
    setBitMask(MFRC522_REG_FIFO_LEVEL, 0x80);
    
    for (uint8_t i = 0; i < length; i++) {
        writeRegister(MFRC522_REG_FIFO_DATA, data[i]);
    }
    writeRegister(MFRC522_REG_COMMAND, PCD_CALCCRC);
    
    uint32_t i = 0xFF;
    uint8_t n;
    do {
        n = readRegister(MFRC522_REG_DIV_IRQ);
        i--;
    } while ((i != 0) && !(n & 0x04));
    
    result[0] = readRegister(MFRC522_REG_CRC_RESULT_L);
    result[1] = readRegister(MFRC522_REG_CRC_RESULT_M);
}

uint8_t Rfid::PCD_Authenticate(uint8_t command, uint8_t blockAddr, uint8_t *key, uint8_t *uid) {
    uint8_t sendData[12];
    sendData[0] = command;
    sendData[1] = blockAddr;
    for (uint8_t i = 0; i < 6; i++) sendData[2 + i] = key[i];
    for (uint8_t i = 0; i < 4; i++) sendData[8 + i] = uid[i];
    
    uint32_t backLen = 0;
    uint8_t status = toCard(PCD_AUTHENT, sendData, 12, sendData, &backLen);
    
    if ((readRegister(MFRC522_REG_STATUS2) & 0x08) == 0) {
        status = STATUS_ERROR;
    }
    return status;
}

uint8_t Rfid::MIFARE_Read(uint8_t blockAddr, uint8_t *buffer, uint8_t *bufferSize) {
    uint8_t sendData[4];
    sendData[0] = PICC_CMD_MF_READ;
    sendData[1] = blockAddr;
    calculateCRC(sendData, 2, &sendData[2]);
    
    uint32_t backLen = 0;
    uint8_t status = toCard(PCD_TRANSCEIVE, sendData, 4, buffer, &backLen);
    
    if (status == STATUS_OK && backLen >= 128) {
        *bufferSize = 16;
        return STATUS_OK;
    }
    return STATUS_ERROR;
}

uint8_t Rfid::MIFARE_Write(uint8_t blockAddr, uint8_t *buffer, uint8_t bufferSize) {
    uint8_t sendData[18];
    sendData[0] = PICC_CMD_MF_WRITE;
    sendData[1] = blockAddr;
    calculateCRC(sendData, 2, &sendData[2]);
    
    uint32_t backLen = 0;
    uint8_t status = toCard(PCD_TRANSCEIVE, sendData, 4, sendData, &backLen);
    
    if (status != STATUS_OK || (backLen != 4) || ((sendData[0] & 0x0F) != 0x0A)) {
        return STATUS_ERROR;
    }
    for (uint8_t i = 0; i < 16; i++) {
        sendData[i] = buffer[i];
    }
    calculateCRC(sendData, 16, &sendData[16]);
    status = toCard(PCD_TRANSCEIVE, sendData, 18, sendData, &backLen);
    
    if (status != STATUS_OK || (backLen != 4) || ((sendData[0] & 0x0F) != 0x0A)) {
        return STATUS_ERROR;
    }
    return STATUS_OK;
}

void Rfid::PICC_HaltA() {
    uint8_t sendData[4];
    sendData[0] = PICC_CMD_HALT;
    sendData[1] = 0;
    calculateCRC(sendData, 2, &sendData[2]);
    
    uint32_t backLen = 0;
    toCard(PCD_TRANSCEIVE, sendData, 4, sendData, &backLen);
}

void Rfid::PCD_StopCrypto1() {
    clearBitMask(MFRC522_REG_STATUS2, 0x08);
}
uint8_t Rfid::selectTag(uint8_t* serial) {
    uint8_t status;
    uint8_t size;
    uint32_t recvBits;
    uint8_t buffer[9]; 

    buffer[0] = PICC_ANTICOLL; 
    buffer[1] = 0x70;        
    buffer[2] = serial[0];     
    buffer[3] = serial[1];
    buffer[4] = serial[2];
    buffer[5] = serial[3];
    buffer[6] = serial[4]; 
    
    calculateCRC(buffer, 7, &buffer[7]);
    
    status = toCard(PCD_TRANSCEIVE, buffer, 9, buffer, &recvBits);
    
    if ((status == STATUS_OK) && (recvBits == 0x18)) {
        size = buffer[0];
        return size; 
    } else {
        return 0; 
    }
}