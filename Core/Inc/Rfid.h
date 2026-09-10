#ifndef RFID_H
#define RFID_H

#include "stm32f1xx.h"
#include "SpiBus.h"

// --- REGISTERS ---
#define MFRC522_REG_COMMAND       0x01
#define MFRC522_REG_COM_I_EN      0x02
#define MFRC522_REG_DIV_I_EN      0x03
#define MFRC522_REG_COM_IRQ       0x04
#define MFRC522_REG_DIV_IRQ       0x05
#define MFRC522_REG_ERROR         0x06
#define MFRC522_REG_STATUS1       0x07
#define MFRC522_REG_STATUS2       0x08
#define MFRC522_REG_FIFO_DATA     0x09
#define MFRC522_REG_FIFO_LEVEL    0x0A
#define MFRC522_REG_CONTROL       0x0C
#define MFRC522_REG_BIT_FRAMING   0x0D
#define MFRC522_REG_COLL          0x0E
#define MFRC522_REG_MODE          0x11
#define MFRC522_REG_TX_CONTROL    0x14
#define MFRC522_REG_TX_AUTO       0x15
#define MFRC522_REG_RF_CFG        0x26
#define MFRC522_REG_CRC_RESULT_M  0x21
#define MFRC522_REG_CRC_RESULT_L  0x22
#define MFRC522_REG_T_MODE        0x2A
#define MFRC522_REG_T_PRESCALER   0x2B
#define MFRC522_REG_T_RELOAD_H    0x2C
#define MFRC522_REG_T_RELOAD_L    0x2D

#define PCD_IDLE                  0x00
#define PCD_CALCCRC               0x03
#define PCD_TRANSCEIVE            0x0C
#define PCD_AUTHENT               0x0E
#define PCD_SOFTRESET             0x0F

#define PICC_REQIDL               0x26 
#define PICC_ANTICOLL             0x93 

#define PICC_CMD_MF_AUTH_KEY_A    0x60
#define PICC_CMD_MF_AUTH_KEY_B    0x61
#define PICC_CMD_MF_READ          0x30
#define PICC_CMD_MF_WRITE         0xA0
#define PICC_CMD_HALT             0x50

#define STATUS_OK                 1
#define STATUS_ERROR              0

class Rfid {
private:
    SpiBus* spi_bus;
    GPIO_TypeDef* csPort;   uint16_t csPin;
    GPIO_TypeDef* rstPort;  uint16_t rstPin;

    void csLow();
    void csHigh();
    void rstLow();
    void rstHigh();

    void writeRegister(uint8_t addr, uint8_t val);
    uint8_t readRegister(uint8_t addr);
    void setBitMask(uint8_t reg, uint8_t mask);
    void clearBitMask(uint8_t reg, uint8_t mask);
    void antennaOn();
    void resetPCD();
    uint8_t toCard(uint8_t command, uint8_t* sendData, uint8_t sendLen, uint8_t* backData, uint32_t* backLen);
    void calculateCRC(uint8_t *data, uint8_t length, uint8_t *result);

public:
    Rfid(SpiBus* spi_bus, 
         GPIO_TypeDef* csPort, uint16_t csPin, 
         GPIO_TypeDef* rstPort, uint16_t rstPin);

    // Khởi tạo, cấu hình chân GPIO và reset module RC522
    void init();
    
    // Phát sóng kiểm tra xem có thẻ RFID trong vùng phủ sóng không (trả về true nếu có)
    bool checkCard();
    
    // Đọc mã định danh UID của thẻ (yêu cầu mảng serial tối thiểu 5 byte để chứa 4 byte UID + 1 byte CRC)
    bool readCardSerial(uint8_t* serial);
    
    // Chọn thẻ để giao tiếp (truyền vào UID vừa đọc), bắt buộc gọi trước khi xác thực mật khẩu
    uint8_t selectTag(uint8_t* serial);
    
    // Xác thực mật khẩu để mở khóa 1 Block (VD: PICC_CMD_MF_AUTH_KEY_A, truyền mảng key 6 byte mặc định 0xFF)
    uint8_t PCD_Authenticate(uint8_t command, uint8_t blockAddr, uint8_t *key, uint8_t *uid);
    
    // Đọc 16 byte dữ liệu từ blockAddr (yêu cầu mảng buffer phải đủ 18 byte do RC522 trả về 2 byte CRC)
    uint8_t MIFARE_Read(uint8_t blockAddr, uint8_t *buffer, uint8_t *bufferSize);
    
    // Ghi 16 byte dữ liệu đè lên blockAddr (yêu cầu mảng buffer phải có đúng 16 byte)
    uint8_t MIFARE_Write(uint8_t blockAddr, uint8_t *buffer, uint8_t bufferSize);
    
    // Đưa thẻ vào trạng thái "Ngủ" (Halt) để thẻ không bị kích hoạt đọc lại liên tục khi vẫn đặt trên đầu đọc
    void PICC_HaltA();
    
    // Tắt chế độ mã hóa Crypto, giải phóng chip RC522. Bắt buộc gọi sau khi Đọc/Ghi xong để chống treo
    void PCD_StopCrypto1();
};

#endif

