#include "SdCard.h"
#include "SystemTimer.h"

#define SD_DUMMY_BYTE 0xFF

SdCard* SdCard::instance = nullptr;

SdCard::SdCard(SpiBus* spi_bus, GPIO_TypeDef* cs_port, uint16_t cs_pin, GPIO_TypeDef* cdPort, uint16_t cdPin, GPIO_TypeDef* pwrPort, uint16_t pwrPin) {
    this->spi_bus = spi_bus;
    this->cs_port = cs_port;
    this->cs_pin = cs_pin;
    this->cdPort = cdPort;
    this->cdPin = cdPin;
    this->pwrPort = pwrPort;
    this->pwrPin = pwrPin;
    this->stat = 0x01;
    this->cardType = 0;
}

void SdCard::init() {
    spi_bus->Init(); // Khởi tạo SPI và các chân GPIO của SPI
    if (cs_port == GPIOA || cdPort == GPIOA || pwrPort == GPIOA) RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
    if (cs_port == GPIOB || cdPort == GPIOB || pwrPort == GPIOB) RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
    if (cs_port == GPIOC || cdPort == GPIOC || pwrPort == GPIOC) RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
    if (cs_port == GPIOD || cdPort == GPIOD || pwrPort == GPIOD) RCC->APB2ENR |= RCC_APB2ENR_IOPDEN;
    
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;

    GPIO_TypeDef* outPorts[] = {cs_port, pwrPort};
    uint16_t outPins[] = {cs_pin, pwrPin};
    
    for (int i = 0; i < 2; i++) {
        uint32_t pinIndex = 0;
        while (!(outPins[i] & (1 << pinIndex))) pinIndex++;
        if (pinIndex < 8) {
            outPorts[i]->CRL &= ~(0xF << (pinIndex * 4));
            outPorts[i]->CRL |= (0x3 << (pinIndex * 4)); 
        } else {
            outPorts[i]->CRH &= ~(0xF << ((pinIndex - 8) * 4));
            outPorts[i]->CRH |= (0x3 << ((pinIndex - 8) * 4)); 
        }
    }
    
    uint32_t pinIndex = 0;
    while (!(cdPin & (1 << pinIndex))) pinIndex++;
    if (pinIndex < 8) {
        cdPort->CRL &= ~(0xF << (pinIndex * 4));
        cdPort->CRL |= (0x8 << (pinIndex * 4)); 
    } else {
        cdPort->CRH &= ~(0xF << ((pinIndex - 8) * 4));
        cdPort->CRH |= (0x8 << ((pinIndex - 8) * 4)); 
    }
    cdPort->ODR |= cdPin;

    csHigh();
    powerOn();
}

bool SdCard::checkPhysicalPresence() {
    int debounce = 0;
    for (int i = 0; i < 10; i++) {
        if ((cdPort->IDR & cdPin) == 0) debounce++;
        SystemTimer::delay_ms(1);
    }
    return (debounce > 8);
}

void SdCard::powerOn() {
    pwrPort->BRR = pwrPin; 
    SystemTimer::delay_ms(50);
}

void SdCard::powerOff() {
    pwrPort->BSRR = pwrPin;
    SystemTimer::delay_ms(50);
}

void SdCard::csLow() {
    spi_bus->Transfer(SD_DUMMY_BYTE);
    cs_port->BRR = cs_pin;
}

void SdCard::csHigh() {
    cs_port->BSRR = cs_pin;
    spi_bus->Transfer(SD_DUMMY_BYTE);
}

void SdCard::setSpiSpeedSlow() {
    spi_bus->SetPrescaler(SPI_BAUDRATEPRESCALER_256);
}

void SdCard::setSpiSpeedFast() {
    spi_bus->SetPrescaler(SPI_BAUDRATEPRESCALER_8);
}

uint8_t SdCard::waitReady() {
    uint8_t res;
    uint32_t tickstart = SystemTimer::getTick();
    do {
        res = spi_bus->Transfer(SD_DUMMY_BYTE);
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

    spi_bus->Transfer(0x40 | cmd);
    spi_bus->Transfer((uint8_t)(arg >> 24));
    spi_bus->Transfer((uint8_t)(arg >> 16));
    spi_bus->Transfer((uint8_t)(arg >> 8));
    spi_bus->Transfer((uint8_t)arg);

    n = 0x01;
    if (cmd == 0) n = 0x95;
    if (cmd == 8) n = 0x87;
    spi_bus->Transfer(n);

    if (cmd == 12) spi_bus->Transfer(SD_DUMMY_BYTE);

    n = 100;
    do { res = spi_bus->Transfer(SD_DUMMY_BYTE); } while ((res & 0x80) && --n);
    return res;
}

uint8_t SdCard::initialize() {
    stat = 0x01;
    if (!checkPhysicalPresence()) {
        stat = 0x03;
        return stat;
    } 
   
    setSpiSpeedSlow(); 
    csHigh();
    SystemTimer::delay_ms(10); 
    
    for (int i = 0; i < 100; i++) {
        spi_bus->Transfer(SD_DUMMY_BYTE);
    }
    
    uint8_t cmd0_res = 0xFF;
    for (int retry = 0; retry < 100; retry++) {
        csLow();
        cmd0_res = sendCmd(0, 0); 
        if (cmd0_res == 1) break;
        csHigh();
        SystemTimer::delay_ms(10);
    }
    
    cardType = 0;
    if (cmd0_res == 1) {
        if (sendCmd(8, 0x1AA) == 1) { 
            uint8_t ocr[4];
            for (int i = 0; i < 4; i++) ocr[i] = spi_bus->Transfer(SD_DUMMY_BYTE); 
            
            if (ocr[2] == 0x01 && ocr[3] == 0xAA) {
                uint32_t tickstart = SystemTimer::getTick();
                do {
                    sendCmd(55, 0);
                    if (sendCmd(41, 1UL << 30) == 0) { 
                        stat &= ~0x01; 
                        break;
                    }
                } while ((SystemTimer::getTick() - tickstart) < 2000);
                
                if (!(stat & 0x01) && sendCmd(58, 0) == 0) {
                    for (int i = 0; i < 4; i++) ocr[i] = spi_bus->Transfer(SD_DUMMY_BYTE);
                    if (ocr[0] & 0x40) {
                        cardType = 1;
                    }
                }
            }
        } else {
            uint32_t tickstart = SystemTimer::getTick();
            do {
                sendCmd(55, 0);
                if (sendCmd(41, 0) == 0) { 
                    stat &= ~0x01; 
                    break;
                }
            } while ((SystemTimer::getTick() - tickstart) < 2000);
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
            while (spi_bus->Transfer(SD_DUMMY_BYTE) != 0xFE) {
                if ((SystemTimer::getTick() - tickstart) > 500) { csHigh(); return 1; }
            }
            for (int i = 0; i < 512; i++) buff[i] = spi_bus->Transfer(SD_DUMMY_BYTE);
            spi_bus->Transfer(SD_DUMMY_BYTE); spi_bus->Transfer(SD_DUMMY_BYTE); 
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
            spi_bus->Transfer(0xFE);
            for (int i = 0; i < 512; i++) spi_bus->Transfer(buff[i]);
            spi_bus->Transfer(SD_DUMMY_BYTE); spi_bus->Transfer(SD_DUMMY_BYTE); 
            if ((spi_bus->Transfer(SD_DUMMY_BYTE) & 0x1F) == 0x05) {
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


