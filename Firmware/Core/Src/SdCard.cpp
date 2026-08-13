#include "SdCard.h"

#define SD_DUMMY_BYTE 0xFF

static uint32_t current_spi_baudrate = SPI_BAUDRATEPRESCALER_256;

static void applySpiSettings(SPI_HandleTypeDef *hspi) {
    uint32_t timeout = 10000;
    while (__HAL_SPI_GET_FLAG(hspi, SPI_FLAG_BSY) && --timeout);
    
    __HAL_SPI_DISABLE(hspi);

    hspi->Instance->CR1 = SPI_CR1_MSTR | SPI_CR1_SSI | SPI_CR1_SSM | current_spi_baudrate;

    __HAL_SPI_ENABLE(hspi);

    uint32_t tmpreg = AFIO->MAPR;
    tmpreg &= ~AFIO_MAPR_SWJ_CFG_Msk;
    tmpreg |= (0x2 << 24); 
    tmpreg &= ~AFIO_MAPR_SPI1_REMAP;
    AFIO->MAPR = tmpreg;

    volatile uint32_t dummy = hspi->Instance->DR;
    dummy = hspi->Instance->SR;
    (void)dummy;
}

SdCard::SdCard(SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin) {
    this->hspi = hspi;
    this->cs_port = cs_port;
    this->cs_pin = cs_pin;
    this->stat = 0x01;
    this->cardType = 0;
}

void SdCard::init() {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = cs_pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(cs_port, &GPIO_InitStruct);
    HAL_GPIO_WritePin(cs_port, cs_pin, GPIO_PIN_SET);
}

void SdCard::csLow() {
    applySpiSettings(hspi); // Chiếm quyền điều khiển bus
    spiTxRx(SD_DUMMY_BYTE);
    HAL_GPIO_WritePin(cs_port, cs_pin, GPIO_PIN_RESET);
}

void SdCard::csHigh() {
    HAL_GPIO_WritePin(cs_port, cs_pin, GPIO_PIN_SET);
    spiTxRx(SD_DUMMY_BYTE);
}

void SdCard::setSpiSpeedSlow() {
    current_spi_baudrate = SPI_BAUDRATEPRESCALER_256;
    applySpiSettings(hspi);
}

void SdCard::setSpiSpeedFast() {
    current_spi_baudrate = SPI_BAUDRATEPRESCALER_8;
    applySpiSettings(hspi);
}

uint8_t SdCard::spiTxRx(uint8_t data) {
    SPI_TypeDef* spi = hspi->Instance;
    spi->DR = data;
    uint32_t timeout = 10000;
    while (!(spi->SR & SPI_SR_TXE) && --timeout);
    timeout = 10000;
    while (!(spi->SR & SPI_SR_RXNE) && --timeout);
    return (uint8_t)spi->DR;
}

uint8_t SdCard::waitReady() {
    uint8_t res;
    uint32_t tickstart = HAL_GetTick();
    do {
        res = spiTxRx(SD_DUMMY_BYTE);
    } while (res != 0xFF && (HAL_GetTick() - tickstart) < 500);
    return res;
}

uint8_t SdCard::sendCmd(uint8_t cmd, uint32_t arg) {
    uint8_t res, n;
    
    if (cmd & 0x80) {
        cmd &= 0x7F;
        res = sendCmd(55, 0);
        if (res > 1) return res;
    }

    if (cmd != 12) {
        csHigh();
        csLow();
        if (waitReady() != 0xFF) return 0xFF;
    } else {
        waitReady();
    }

    spiTxRx(0x40 | cmd);
    spiTxRx((uint8_t)(arg >> 24));
    spiTxRx((uint8_t)(arg >> 16));
    spiTxRx((uint8_t)(arg >> 8));
    spiTxRx((uint8_t)arg);

    n = 0x01;
    if (cmd == 0) n = 0x95;
    if (cmd == 8) n = 0x87;
    spiTxRx(n);

    if (cmd == 12) spiTxRx(SD_DUMMY_BYTE);

    n = 100;
    do { res = spiTxRx(SD_DUMMY_BYTE); } while ((res & 0x80) && --n);
    return res;
}

uint8_t SdCard::initialize() {
    stat = 0x01; 
   
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_5 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP; 
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    HAL_GPIO_WritePin(cs_port, cs_pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_SET); 

    for(int i = 0; i < 100; i++) {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
        for(volatile int j = 0; j < 20; j++); 
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
        for(volatile int j = 0; j < 20; j++);
    }

    // Trả lại cấu hình cho Hardware SPI
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    setSpiSpeedSlow(); 
    csHigh();
    HAL_Delay(10); 
    
    for (int i = 0; i < 100; i++) {
        spiTxRx(SD_DUMMY_BYTE);
    }
    
    uint8_t cmd0_res = 0xFF;
    for (int retry = 0; retry < 10; retry++) { 
        csLow();
        cmd0_res = sendCmd(0, 0); 
        if (cmd0_res == 1) break;
        csHigh();
        HAL_Delay(10);
    }
    
    cardType = 0;
    if (cmd0_res == 1) {
        if (sendCmd(8, 0x1AA) == 1) { 
            for (int i = 0; i < 4; i++) spiTxRx(SD_DUMMY_BYTE); 
            uint32_t tickstart = HAL_GetTick();
            do {
                sendCmd(55, 0);
                if (sendCmd(41, 1UL << 30) == 0) { 
                    stat &= ~0x01; 
                    break;
                }
            } while ((HAL_GetTick() - tickstart) < 1000); 
            
            if (sendCmd(58, 0) == 0) {
                uint8_t ocr[4];
                for (int i = 0; i < 4; i++) ocr[i] = spiTxRx(SD_DUMMY_BYTE);
                if (ocr[0] & 0x40) cardType = 1; 
            }
        } else {
            uint32_t tickstart = HAL_GetTick();
            do {
                sendCmd(55, 0);
                if (sendCmd(41, 0) == 0) { 
                    stat &= ~0x01; 
                    break;
                }
            } while ((HAL_GetTick() - tickstart) < 1000); 
        }
    }
    
    csHigh();
    setSpiSpeedFast(); 
    
    return stat;
}

uint8_t SdCard::status() { return stat; }

uint8_t SdCard::read(uint8_t *buff, uint32_t sector, uint32_t count) {
    if (stat & 0x01) return 1; 
    if (cardType == 0) sector *= 512;
    uint8_t res = 1; 
    csLow();
    while (count > 0) {
        if (sendCmd(17, sector) == 0) { 
            uint32_t tickstart = HAL_GetTick();
            while (spiTxRx(SD_DUMMY_BYTE) != 0xFE) {
                if ((HAL_GetTick() - tickstart) > 500) { csHigh(); return 1; }
            }
            for (int i = 0; i < 512; i++) buff[i] = spiTxRx(SD_DUMMY_BYTE);
            spiTxRx(SD_DUMMY_BYTE); spiTxRx(SD_DUMMY_BYTE); 
            if (cardType == 0) sector += 512; else sector += 1;
            buff += 512; count--; res = 0; 
        } else { res = 1; break; }
    }
    csHigh();
    return res;
}

uint8_t SdCard::write(const uint8_t *buff, uint32_t sector, uint32_t count) {
    if (stat & 0x01) return 1; 
    if (cardType == 0) sector *= 512;
    uint8_t res = 1; 
    csLow();
    while (count > 0) {
        if (sendCmd(24, sector) == 0) { 
            spiTxRx(0xFE);
            for (int i = 0; i < 512; i++) spiTxRx(buff[i]);
            spiTxRx(SD_DUMMY_BYTE); spiTxRx(SD_DUMMY_BYTE); 
            if ((spiTxRx(SD_DUMMY_BYTE) & 0x1F) == 0x05) {
                waitReady();
                if (cardType == 0) sector += 512; else sector += 1;
                buff += 512; count--; res = 0; 
            } else break;
        } else { res = 1; break; }
    }
    csHigh();
    return res;
}

uint8_t SdCard::ioctl(uint8_t cmd, void *buff) {
    if (stat & 0x01) return 1; 
    uint8_t res = 1; 
    switch (cmd) {
        case 0: csLow(); if (waitReady() == 0xFF) res = 0; csHigh(); break;
        case 1: break;
        case 2: *(uint16_t*)buff = 512; res = 0; break;
        case 3: *(uint32_t*)buff = 1; res = 0; break;
        default: res = 4; break;
    }
    return res;
}

extern "C" {
    extern SdCard sdCard; 
    uint8_t sdCard_initialize(void) { return sdCard.initialize(); }
    uint8_t sdCard_status(void) { return sdCard.status(); }
    uint8_t sdCard_read(uint8_t *buff, uint32_t sector, uint32_t count) { return sdCard.read(buff, sector, count); }
    uint8_t sdCard_write(const uint8_t *buff, uint32_t sector, uint32_t count) { return sdCard.write(buff, sector, count); }
    uint8_t sdCard_ioctl(uint8_t cmd, void *buff) { return sdCard.ioctl(cmd, buff); }
}