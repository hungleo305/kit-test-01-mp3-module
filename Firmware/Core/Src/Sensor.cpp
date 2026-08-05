#include "Sensor.h"
#include "SystemTimer.h"

volatile uint16_t Sensor::productCount = 0;
volatile bool Sensor::isDetected = false;
uint32_t Sensor::lastInterruptTick = 0;

void Sensor::init() {
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN | RCC_APB2ENR_AFIOEN;
    
    GPIOC->CRH &= ~(0xF << 20);
    GPIOC->CRH |= (0x8 << 20);  
    GPIOC->ODR |= (1 << 13);    
    
    AFIO->EXTICR[3] &= ~(0xF << 4);
    AFIO->EXTICR[3] |= (0x2 << 4);
    
    EXTI->IMR |= (1 << 13);
    
    EXTI->RTSR |= (1 << 13);
    
    NVIC_SetPriority(EXTI15_10_IRQn, 2);
    NVIC_EnableIRQ(EXTI15_10_IRQn);
}

void Sensor::handleInterrupt() {
    if (EXTI->PR & (1 << 13)) {
        
        uint32_t currentTick = SystemTimer::getTick();
        
        if (currentTick - lastInterruptTick > 200) {
            productCount++;
            isDetected = true;
            lastInterruptTick = currentTick;
        }
        EXTI->PR |= (1 << 13);
    }
}
extern "C" void EXTI15_10_IRQHandler(void) {
    Sensor::handleInterrupt();
}