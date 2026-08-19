#ifndef __SDCARD_H
#define __SDCARD_H

#include "stm32f1xx.h"

#ifdef __cplusplus
class SdCard {
private:
    SPI_TypeDef *spi;
    GPIO_TypeDef *cs_port;
    uint16_t cs_pin;
    uint8_t stat;
    uint8_t cardType;

    void setSpiSpeedSlow();
    void setSpiSpeedFast();

    void csLow();
    void csHigh();

    uint8_t spiTxRx(uint8_t data);
    uint8_t waitReady();
    uint8_t sendCmd(uint8_t cmd, uint32_t arg);

public:
    SdCard(SPI_TypeDef *spi, GPIO_TypeDef *cs_port, uint16_t cs_pin);
    
    void init();
    uint8_t initialize();
    uint8_t status();
    uint8_t read(uint8_t *buff, uint32_t sector, uint32_t count);
    uint8_t write(const uint8_t *buff, uint32_t sector, uint32_t count);
    uint8_t ioctl(uint8_t cmd, void *buff);
};
#endif

#ifdef __cplusplus
extern "C" {
#endif

uint8_t sdCard_initialize(void);
uint8_t sdCard_status(void);
uint8_t sdCard_read(uint8_t *buff, uint32_t sector, uint32_t count);
uint8_t sdCard_write(const uint8_t *buff, uint32_t sector, uint32_t count);
uint8_t sdCard_ioctl(uint8_t cmd, void *buff);

#ifdef __cplusplus
}
#endif

#endif
