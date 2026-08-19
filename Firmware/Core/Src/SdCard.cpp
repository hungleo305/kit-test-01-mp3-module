#include "SdCard.h"
#include "SystemTimer.h"

#define SD_DUMMY_BYTE 0xFF

static uint32_t current_spi_baudrate = SPI_BAUDRATEPRESCALER_256;

static void applySpiSettings(SPI_TypeDef *spi) {
    uint32_t timeout = 10000;
    while ((spi->SR & SPI_SR_BSY) && --timeout);
    
    spi->CR1 &= ~SPI_CR1_SPE;

    spi->CR1 = SPI_CR1_MSTR | SPI_CR1_SSI | SPI_CR1_SSM | current_spi_baudrate;

    spi->CR1 |= SPI_CR1_SPE;

    uint32_t tmpreg = AFIO->MAPR;
    tmpreg &= ~AFIO_MAPR_SWJ_CFG_Msk;
    tmpreg |= (0x2 << 24); 
    if (spi == SPI1) {
        tmpreg &= ~AFIO_MAPR_SPI1_REMAP; // Gỡ remap cho SPI1 để về PA5-6-7
    }
    AFIO->MAPR = tmpreg;

    volatile uint32_t dummy = spi->DR;
    dummy = spi->SR;
    (void)dummy;
}

SdCard::SdCard(SPI_TypeDef *spi, GPIO_TypeDef *cs_port, uint16_t cs_pin) {
    this->spi = spi;
    this->cs_port = cs_port;
    this->cs_pin = cs_pin;
    this->stat = 0x01;
    this->cardType = 0;
}

void SdCard::init() {
    // 1. Enable Clocks
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN | RCC_APB2ENR_AFIOEN;
    if (spi == SPI1) {
        RCC->APB2ENR |= RCC_APB2ENR_SPI1EN | RCC_APB2ENR_IOPAEN;
    } else if (spi == SPI2) {
        RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;
        RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
    }

    // 2. Configure CS Pin (PC8) as Output Push-Pull 50MHz
    if (cs_port == GPIOC && cs_pin == (1<<8)) {
        GPIOC->CRH &= ~(0xF << 0);
        GPIOC->CRH |=  (0x3 << 0);
    } else {
        // Fallback for general purpose (not perfectly generic but works for this project)
        // Would need complex port matching to be generic
    }

    csHigh();
}

void SdCard::csLow() {
    applySpiSettings(spi);
    spiTxRx(SD_DUMMY_BYTE);
    cs_port->BRR = cs_pin;
}

void SdCard::csHigh() {
    cs_port->BSRR = cs_pin;
    spiTxRx(SD_DUMMY_BYTE);
}

void SdCard::setSpiSpeedSlow() {
    current_spi_baudrate = SPI_BAUDRATEPRESCALER_256;
    applySpiSettings(spi);
}

void SdCard::setSpiSpeedFast() {
    current_spi_baudrate = SPI_BAUDRATEPRESCALER_8;
    applySpiSettings(spi);
}

uint8_t SdCard::spiTxRx(uint8_t data) {
    spi->DR = data;
    uint32_t timeout = 10000;
    while (!(spi->SR & SPI_SR_TXE) && --timeout);
    timeout = 10000;
    while (!(spi->SR & SPI_SR_RXNE) && --timeout);
    return (uint8_t)spi->DR;
}

uint8_t SdCard::waitReady() {
    uint8_t res;
    uint32_t tickstart = SystemTimer::getTick();
    do {
        res = spiTxRx(SD_DUMMY_BYTE);
    } while (res != 0xFF && (SystemTimer::getTick() - tickstart) < 500);
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
   
    // Ensure SCK, MISO, MOSI are initialized based on SPI selected
    if (spi == SPI1) {
        // PA5 (SCK) & PA7 (MOSI) Alt Func PP 50MHz, PA6 (MISO) Input Pull-Up
        GPIOA->CRL &= ~(0xF << 20 | 0xF << 24 | 0xF << 28);
        GPIOA->CRL |=  (0xB << 20 | 0x8 << 24 | 0xB << 28);
        GPIOA->ODR |= (1 << 6); // Pull-up for MISO
    } else if (spi == SPI2) {
        // PB13 (SCK) & PB15 (MOSI) Alt Func PP 50MHz, PB14 (MISO) Input Pull-Up
        GPIOB->CRH &= ~(0xF << 20 | 0xF << 24 | 0xF << 28);
        GPIOB->CRH |=  (0xB << 20 | 0x8 << 24 | 0xB << 28);
        GPIOB->ODR |= (1 << 14); // Pull-up for MISO
    }

    setSpiSpeedSlow(); 
    csHigh();
    SystemTimer::delay_ms(10); 
    
    // At least 74 dummy clocks with CS high
    for (int i = 0; i < 100; i++) {
        spiTxRx(SD_DUMMY_BYTE);
    }
    
    uint8_t cmd0_res = 0xFF;
    for (int retry = 0; retry < 10; retry++) { 
        csLow();
        cmd0_res = sendCmd(0, 0); 
        if (cmd0_res == 1) break;
        csHigh();
        SystemTimer::delay_ms(10);
    }
    
    cardType = 0;
    if (cmd0_res == 1) {
        if (sendCmd(8, 0x1AA) == 1) { 
            for (int i = 0; i < 4; i++) spiTxRx(SD_DUMMY_BYTE); 
            uint32_t tickstart = SystemTimer::getTick();
            do {
                sendCmd(55, 0);
                if (sendCmd(41, 1UL << 30) == 0) { 
                    stat &= ~0x01; 
                    break;
                }
            } while ((SystemTimer::getTick() - tickstart) < 1000); 
            
            if (sendCmd(58, 0) == 0) {
                uint8_t ocr[4];
                for (int i = 0; i < 4; i++) ocr[i] = spiTxRx(SD_DUMMY_BYTE);
                if (ocr[0] & 0x40) cardType = 1; 
            }
        } else {
            uint32_t tickstart = SystemTimer::getTick();
            do {
                sendCmd(55, 0);
                if (sendCmd(41, 0) == 0) { 
                    stat &= ~0x01; 
                    break;
                }
            } while ((SystemTimer::getTick() - tickstart) < 1000); 
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
            uint32_t tickstart = SystemTimer::getTick();
            while (spiTxRx(SD_DUMMY_BYTE) != 0xFE) {
                if ((SystemTimer::getTick() - tickstart) > 500) { csHigh(); return 1; }
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