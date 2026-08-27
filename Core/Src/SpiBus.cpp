#include "SpiBus.h"

// 1a. Khởi tạo Hardware SPI (Dùng cấu hình có sẵn từ CubeMX)
SpiBus::SpiBus(SPI_TypeDef* spi_instance)
    : isSoftware(false), hspi(spi_instance),
      sckPort(nullptr), sckPin(0),
      misoPort(nullptr), misoPin(0),
      mosiPort(nullptr), mosiPin(0),
      delay_ticks(0) {
}

// 1b. Khởi tạo Hardware SPI (Bare-metal)
SpiBus::SpiBus(SPI_TypeDef* spi_instance, 
               GPIO_TypeDef* sckPort, uint16_t sckPin,
               GPIO_TypeDef* misoPort, uint16_t misoPin,
               GPIO_TypeDef* mosiPort, uint16_t mosiPin)
    : isSoftware(false), hspi(spi_instance),
      sckPort(sckPort), sckPin(sckPin),
      misoPort(misoPort), misoPin(misoPin),
      mosiPort(mosiPort), mosiPin(mosiPin),
      delay_ticks(0) {
}

// 2. Khởi tạo Software SPI
SpiBus::SpiBus(GPIO_TypeDef* sckPort, uint16_t sckPin,
               GPIO_TypeDef* misoPort, uint16_t misoPin,
               GPIO_TypeDef* mosiPort, uint16_t mosiPin)
    : isSoftware(true), hspi(nullptr),
      sckPort(sckPort), sckPin(sckPin),
      misoPort(misoPort), misoPin(misoPin),
      mosiPort(mosiPort), mosiPin(mosiPin),
      delay_ticks(10) {
}

void SpiBus::delay() {
    for (volatile uint32_t i = 0; i < delay_ticks; i++);
}

static void enable_gpio_clock(GPIO_TypeDef* port) {
    if (port == GPIOA) RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
    else if (port == GPIOB) RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
    else if (port == GPIOC) RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
    else if (port == GPIOD) RCC->APB2ENR |= RCC_APB2ENR_IOPDEN;
}

void SpiBus::Init() {
    if (hspi != nullptr && sckPort == nullptr) {
        // Nếu truyền SPI cứng nhưng không truyền chân GPIO, nghĩa là 
        // người dùng đã cấu hình mọi thứ từ CubeMX -> Bỏ qua tự động khởi tạo.
        // NHƯNG HAL_SPI_Init không tự bật SPE, nên ta phải bật nó ở đây.
        hspi->CR1 |= SPI_CR1_SPE;
        return; 
    }

    enable_gpio_clock(sckPort);
    enable_gpio_clock(misoPort);
    enable_gpio_clock(mosiPort);
    
    GPIO_TypeDef* ports[] = {sckPort, misoPort, mosiPort};
    uint16_t pins[] = {sckPin, misoPin, mosiPin};
    
    if (isSoftware) {
        uint8_t modes[] = {0x3, 0x4, 0x3}; // SCK: Output PP, MISO: Input Float, MOSI: Output PP
        for (int i = 0; i < 3; i++) {
            uint32_t pinIndex = 0;
            while (!(pins[i] & (1 << pinIndex))) pinIndex++;
            if (pinIndex < 8) {
                ports[i]->CRL &= ~(0xF << (pinIndex * 4));
                ports[i]->CRL |= (modes[i] << (pinIndex * 4)); 
            } else {
                ports[i]->CRH &= ~(0xF << ((pinIndex - 8) * 4));
                ports[i]->CRH |= (modes[i] << ((pinIndex - 8) * 4));
            }
        }
        sckPort->BRR = sckPin; // SCK LOW
    } else {
        // Bật Clock SPI tương ứng
        if (hspi == SPI1) RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
#ifdef SPI2
        else if (hspi == SPI2) RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;
#endif
#ifdef SPI3
        else if (hspi == SPI3) RCC->APB1ENR |= RCC_APB1ENR_SPI3EN;
#endif
        
        // SCK (AF_PP 0xB), MISO (Input Float 0x4 hoặc PU), MOSI (AF_PP 0xB)
        uint8_t modes[] = {0xB, 0x4, 0xB}; 
        for (int i = 0; i < 3; i++) {
            uint32_t pinIndex = 0;
            while (!(pins[i] & (1 << pinIndex))) pinIndex++;
            if (pinIndex < 8) {
                ports[i]->CRL &= ~(0xF << (pinIndex * 4));
                ports[i]->CRL |= (modes[i] << (pinIndex * 4)); 
            } else {
                ports[i]->CRH &= ~(0xF << ((pinIndex - 8) * 4));
                ports[i]->CRH |= (modes[i] << ((pinIndex - 8) * 4));
            }
        }
        
        // Nếu là SPI3 thì có thể cần Remap tùy phần cứng, ở đây giả định chung.
        // Cấu hình CR1
        hspi->CR1 = 0x0000;
        // Master, BR /16 (mặc định), Soft NSS, CPOL=0, CPHA=0
        hspi->CR1 |= SPI_CR1_MSTR | SPI_CR1_BR_1 | SPI_CR1_BR_0 | SPI_CR1_SSM | SPI_CR1_SSI;
        hspi->CR1 |= SPI_CR1_SPE;
    }
}

uint8_t SpiBus::Transfer(uint8_t data) {
    if (isSoftware) {
        uint8_t rxData = 0;
        for (int i = 0; i < 8; i++) {
            if (data & (1 << (7 - i))) mosiPort->BSRR = mosiPin;
            else mosiPort->BRR = mosiPin;
            
            delay();
            sckPort->BSRR = sckPin;
            delay();
            
            if (misoPort->IDR & misoPin) rxData |= (1 << (7 - i));
            
            sckPort->BRR = sckPin;
            delay();
        }
        return rxData;
    } else {
        uint32_t timeout = 100000;
        while (!(hspi->SR & SPI_SR_TXE) && --timeout);
        *((volatile uint8_t *)&hspi->DR) = data;
        
        timeout = 100000;
        while (!(hspi->SR & SPI_SR_RXNE) && --timeout);
        return *((volatile uint8_t *)&hspi->DR);
    }
}

void SpiBus::TransferBuffer(const uint8_t* tx_data, uint8_t* rx_data, uint16_t size) {
    for (uint16_t i = 0; i < size; i++) {
        uint8_t tx = (tx_data != nullptr) ? tx_data[i] : 0xFF;
        uint8_t rx = Transfer(tx);
        if (rx_data != nullptr) {
            rx_data[i] = rx;
        }
    }
}

void SpiBus::SetSpeed(uint32_t speed_hz) {
    if (isSoftware) {
        if (speed_hz > 1000000) delay_ticks = 2;
        else if (speed_hz > 500000) delay_ticks = 10;
        else delay_ticks = 50;
    }
}

void SpiBus::SetPrescaler(uint32_t prescaler) {
    if (isSoftware) {
        switch (prescaler) {
            case 0: delay_ticks = 2; break; 
            case 1: delay_ticks = 4; break; 
            case 2: delay_ticks = 8; break; 
            case 3: delay_ticks = 16; break; 
            case 4: delay_ticks = 32; break; 
            case 5: delay_ticks = 64; break; 
            case 6: delay_ticks = 128; break; 
            case 7: delay_ticks = 256; break; 
            default: delay_ticks = 10; break;
        }
    } else {
        uint32_t timeout = 10000;
        while ((hspi->SR & SPI_SR_BSY) && --timeout);
        
        hspi->CR1 &= ~SPI_CR1_SPE;
        
        uint32_t cr1 = hspi->CR1;
        cr1 &= ~(7U << 3); 
        cr1 |= (prescaler & (7U << 3));
        hspi->CR1 = cr1;
        
        hspi->CR1 |= SPI_CR1_SPE;
    }
}
