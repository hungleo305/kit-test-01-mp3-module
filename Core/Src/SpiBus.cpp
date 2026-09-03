#include "SpiBus.h"

// 1a. Khởi tạo Hardware SPI (Dùng cấu hình có sẵn từ CubeMX)
SpiBus::SpiBus(SPI_TypeDef* spi_instance)
    : isSoftware(false), hspi(spi_instance),
      sckPort(nullptr), sckPin(0),
      misoPort(nullptr), misoPin(0),
      mosiPort(nullptr), mosiPin(0),
      delay_ticks(0), dma_rx(nullptr), dma_tx(nullptr) {
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
      delay_ticks(0), dma_rx(nullptr), dma_tx(nullptr) {
}

// 2. Khởi tạo Software SPI
SpiBus::SpiBus(GPIO_TypeDef* sckPort, uint16_t sckPin,
               GPIO_TypeDef* misoPort, uint16_t misoPin,
               GPIO_TypeDef* mosiPort, uint16_t mosiPin)
    : isSoftware(true), hspi(nullptr),
      sckPort(sckPort), sckPin(sckPin),
      misoPort(misoPort), misoPin(misoPin),
      mosiPort(mosiPort), mosiPin(mosiPin),
      delay_ticks(10), dma_rx(nullptr), dma_tx(nullptr) {
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
        // Cấu hình SPI
        hspi->CR1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI | SPI_CR1_BR_1 | SPI_CR1_BR_0; 
        
        // Bật DMA clock & gán kênh DMA
        RCC->AHBENR |= RCC_AHBENR_DMA1EN;
        if (hspi == SPI1) {
            dma_rx = DMA1_Channel2;
            dma_tx = DMA1_Channel3;
        } 
#ifdef SPI2
        else if (hspi == SPI2) {
            dma_rx = DMA1_Channel4;
            dma_tx = DMA1_Channel5;
        }
#endif
        
        // NHƯNG HAL_SPI_Init không tự bật SPE, nên ta phải bật nó ở đây.
        hspi->CR1 |= SPI_CR1_SPE;
        return; 
    }

    enable_gpio_clock(sckPort);
    enable_gpio_clock(misoPort);
    enable_gpio_clock(mosiPort);
    
    // Tự động tắt JTAG (giữ lại SWD) nếu vô tình sử dụng các chân cấu hình JTAG (PA15, PB3, PB4, PB5)
    GPIO_TypeDef* ports[] = {sckPort, misoPort, mosiPort};
    uint16_t pins[] = {sckPin, misoPin, mosiPin};
    
    for (int i = 0; i < 3; i++) {
        if ((ports[i] == GPIOB && (pins[i] == (1<<3) || pins[i] == (1<<4) || pins[i] == (1<<5))) ||
            (ports[i] == GPIOA && pins[i] == (1<<15))) {
            RCC->APB2ENR |= RCC_APB2ENR_AFIOEN; // Bật Clock cho AFIO
            AFIO->MAPR = (AFIO->MAPR & ~AFIO_MAPR_SWJ_CFG_Msk) | AFIO_MAPR_SWJ_CFG_JTAGDISABLE;
            break;
        }
    }
    
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
        // Bật Clock SPI tương ứng và gán kênh DMA
        RCC->AHBENR |= RCC_AHBENR_DMA1EN;
        if (hspi == SPI1) {
            RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
            dma_rx = DMA1_Channel2;
            dma_tx = DMA1_Channel3;
        }
#ifdef SPI2
        else if (hspi == SPI2) {
            RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;
            dma_rx = DMA1_Channel4;
            dma_tx = DMA1_Channel5;
        }
#endif
#ifdef SPI3
        else if (hspi == SPI3) {
            RCC->APB1ENR |= RCC_APB1ENR_SPI3EN;
            // SPI3 uses DMA2, ignoring for now if not needed
        }
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

void SpiBus::TransferDMA(const uint8_t* tx_data, uint8_t* rx_data, uint16_t size) {
    if (!use_dma_flag || isSoftware || !dma_rx || !dma_tx) {
        TransferBuffer(tx_data, rx_data, size);
        return;
    }
    
    static const uint8_t dummy_tx = 0xFF;
    static uint8_t dummy_rx = 0;
    
    // Đảm bảo SPI không bận và không có lỗi cũ
    while (hspi->SR & SPI_SR_RXNE) { volatile uint8_t tmpreg = hspi->DR; (void)tmpreg; }
    volatile uint8_t tmpreg = hspi->SR; (void)tmpreg; // Clear OVR
    
    // Tắt DMA cũ
    dma_rx->CCR &= ~DMA_CCR_EN;
    dma_tx->CCR &= ~DMA_CCR_EN;
    
    // Xóa cờ ngắt cũ (CGIF2/3 cho SPI1, 4/5 cho SPI2)
    if (dma_rx == DMA1_Channel2) DMA1->IFCR = DMA_IFCR_CGIF2 | DMA_IFCR_CGIF3;
    else if (dma_rx == DMA1_Channel4) DMA1->IFCR = DMA_IFCR_CGIF4 | DMA_IFCR_CGIF5;
    
    // Cấu hình DMA RX
    dma_rx->CPAR = (uint32_t)&hspi->DR;
    dma_rx->CMAR = rx_data ? (uint32_t)rx_data : (uint32_t)&dummy_rx;
    dma_rx->CNDTR = size;
    dma_rx->CCR = rx_data ? (DMA_CCR_MINC) : (0); // Tăng địa chỉ bộ nhớ nếu rx_data != null
    
    // Cấu hình DMA TX
    dma_tx->CPAR = (uint32_t)&hspi->DR;
    dma_tx->CMAR = tx_data ? (uint32_t)tx_data : (uint32_t)&dummy_tx;
    dma_tx->CNDTR = size;
    dma_tx->CCR = DMA_CCR_DIR | (tx_data ? DMA_CCR_MINC : 0);
    
    // Bật cờ DMA trong SPI
    hspi->CR2 |= SPI_CR2_RXDMAEN | SPI_CR2_TXDMAEN;
    
    // Bật DMA Channel
    dma_rx->CCR |= DMA_CCR_EN;
    dma_tx->CCR |= DMA_CCR_EN;
    
    // Chờ DMA RX xong (Cờ TC - Transfer Complete) có timeout
    uint32_t rx_tc_flag = (dma_rx == DMA1_Channel2) ? DMA_ISR_TCIF2 : DMA_ISR_TCIF4;
    uint32_t timeout = 2000000;
    while (((DMA1->ISR & rx_tc_flag) == 0) && --timeout);
    
    // Đảm bảo SPI đã thực sự truyền xong bit cuối cùng
    timeout = 100000;
    while ((hspi->SR & SPI_SR_BSY) && --timeout);
    
    // Tắt DMA
    dma_rx->CCR &= ~DMA_CCR_EN;
    dma_tx->CCR &= ~DMA_CCR_EN;
    hspi->CR2 &= ~(SPI_CR2_RXDMAEN | SPI_CR2_TXDMAEN);
    
    // Xóa cờ ngắt DMA một lần nữa
    if (dma_rx == DMA1_Channel2) DMA1->IFCR = DMA_IFCR_CGIF2 | DMA_IFCR_CGIF3;
    else if (dma_rx == DMA1_Channel4) DMA1->IFCR = DMA_IFCR_CGIF4 | DMA_IFCR_CGIF5;
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
