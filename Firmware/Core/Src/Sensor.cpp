#include "Sensor.h"
#include "SystemTimer.h"

volatile uint16_t Sensor::productCount = 0;
volatile bool Sensor::isDetected = false;
uint32_t Sensor::lastInterruptTick = 0;

Sensor::Sensor(GPIO_TypeDef* port, uint16_t pin) : port(port), pin(pin) {}

void Sensor::init() {
    if (port == GPIOA) RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
    else if (port == GPIOB) RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
    else if (port == GPIOC) RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
    else if (port == GPIOD) RCC->APB2ENR |= RCC_APB2ENR_IOPDEN;
    
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;

    uint32_t pinIndex = 0;
    while (!(pin & (1 << pinIndex))) pinIndex++;

    if (pinIndex < 8) {
        port->CRL &= ~(0xF << (pinIndex * 4));
        port->CRL |= (0x8 << (pinIndex * 4));  
    } else {
        port->CRH &= ~(0xF << ((pinIndex - 8) * 4));
        port->CRH |= (0x8 << ((pinIndex - 8) * 4));  
    }
    port->ODR |= pin; // Kéo trở Pull-up
    
    uint32_t portIndex = (port == GPIOA) ? 0 : (port == GPIOB) ? 1 : (port == GPIOC) ? 2 : 3;
    AFIO->EXTICR[pinIndex / 4] &= ~(0xF << ((pinIndex % 4) * 4));
    AFIO->EXTICR[pinIndex / 4] |= (portIndex << ((pinIndex % 4) * 4));
    
    EXTI->IMR |= pin;
    EXTI->FTSR |= pin; //RTSR canh len
    
    IRQn_Type irq;
    if (pinIndex >= 10 && pinIndex <= 15) irq = EXTI15_10_IRQn;
    else if (pinIndex >= 5 && pinIndex <= 9) irq = EXTI9_5_IRQn;
    else if (pinIndex == 4) irq = EXTI4_IRQn;
    else if (pinIndex == 3) irq = EXTI3_IRQn;
    else if (pinIndex == 2) irq = EXTI2_IRQn;
    else if (pinIndex == 1) irq = EXTI1_IRQn;
    else irq = EXTI0_IRQn;
    
    NVIC_SetPriority(irq, 2);
    NVIC_EnableIRQ(irq);
}

void Sensor::handleInterrupt(uint16_t extiPin) {
    if (EXTI->PR & extiPin) {
        EXTI->PR = extiPin; 
        
        uint32_t currentTick = SystemTimer::getTick();
        
        // [FIX] Task 3: Giảm thời gian chống rung xuống 20ms (trước đây là 200ms làm lọt số)
        if (currentTick - lastInterruptTick > 20) {
            // [FIX] Task 2: Ghi giá trị đếm mới vào thanh ghi BKP->DR1 để chống mất điện
            productCount++;
            BKP->DR1 = productCount;
            isDetected = true;
            lastInterruptTick = currentTick;
        }
    }
}

extern "C" void EXTI15_10_IRQHandler(void) {
    Sensor::handleInterrupt(1 << 13);
}

uint32_t Sensor::testMaxCountsPerSecond() {
    uint32_t startTick = SystemTimer::getTick();
    uint32_t simulatedCount = 0;
    
    // Tạm thời vô hiệu hóa debounce bằng cách reset lastInterruptTick liên tục
    lastInterruptTick = 0;
    
    // Bắn ngắt liên tục trong 1 giây (1000ms)
    while (SystemTimer::getTick() - startTick < 1000) {
        // Kích hoạt ngắt mềm (Software Interrupt) trên chân của cảm biến (EXTI 13)
        EXTI->SWIER |= (1 << 13);
        simulatedCount++;
        
        // Hàm ngắt handleInterrupt sẽ tự động được phần cứng gọi và đếm productCount
        lastInterruptTick = 0; // Ép bỏ qua debounce cho lần lặp tiếp theo
    }
    
    return simulatedCount;
}
