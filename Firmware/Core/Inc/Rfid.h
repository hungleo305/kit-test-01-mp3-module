#ifndef RFID_H
#define RFID_H

#include "stm32f1xx.h"

// --- Các thanh ghi co b?n c?a RC522 ---
#define MFRC522_REG_COMMAND       0x01
#define MFRC522_REG_COM_IRQ       0x04
#define MFRC522_REG_ERROR         0x06
#define MFRC522_REG_FIFO_DATA     0x09
#define MFRC522_REG_FIFO_LEVEL    0x0A
#define MFRC522_REG_CONTROL       0x0C
#define MFRC522_REG_BIT_FRAMING   0x0D
#define MFRC522_REG_MODE          0x11
#define MFRC522_REG_TX_CONTROL    0x14
#define MFRC522_REG_TX_AUTO       0x15
#define MFRC522_REG_T_MODE        0x2A
#define MFRC522_REG_T_PRESCALER   0x2B
#define MFRC522_REG_T_RELOAD_H    0x2C
#define MFRC522_REG_T_RELOAD_L    0x2D
#define MFRC522_REG_COM_I_EN      0x02
#define MFRC522_REG_STATUS2       0x08
#define MFRC522_REG_COLL          0x0E

// --- T?p l?nh RC522 ---
#define PCD_IDLE                  0x00
#define PCD_TRANSCEIVE            0x0C
#define PICC_REQIDL               0x26 // L?nh tìm th?
#define PICC_ANTICOLL             0x93 // L?nh d?c UID (Ch?ng va ch?m)

class Rfid {
public:
    void init();
    bool checkCard();
    bool readCardSerial(uint8_t* serial);

private:
    uint8_t spiTransfer(uint8_t data);
    void writeRegister(uint8_t addr, uint8_t val);
    uint8_t readRegister(uint8_t addr);
    void setBitMask(uint8_t reg, uint8_t mask);
    void clearBitMask(uint8_t reg, uint8_t mask);
    void antennaOn();
    void resetPCD();
    uint8_t toCard(uint8_t command, uint8_t* sendData, uint8_t sendLen, uint8_t* backData, uint32_t* backLen);
};

#endif