#include "Sensor.h"
#include "SystemTimer.h"



void Sensor::enableDiagnosticMode(bool enable) {
    isDiagnosticMode = enable;
    if (enable) {
        EXTI->RTSR |= pin; // Bật thêm ngắt cạnh lên
    } else {
        EXTI->RTSR &= ~pin; // Tắt ngắt cạnh lên, trở về bình thường
    }
}

bool Sensor::getDiagnosticData(uint16_t& currBlock, uint16_t& currGap, uint16_t& prevBlock, uint16_t& prevGap) {
    if (newDataReady) {
        currBlock = (blockDuration > 9999) ? 9999 : blockDuration;
        currGap = (gapDuration > 9999) ? 9999 : gapDuration;
        prevBlock = (prevBlockDuration > 9999) ? 9999 : prevBlockDuration;
        prevGap = (prevGapDuration > 9999) ? 9999 : prevGapDuration;
        newDataReady = false;
        return true;
    }
    return false;
}

Sensor::Sensor(GPIO_TypeDef* port, uint16_t pin) : port(port), pin(pin), lastInterruptTick(0), isDiagnosticMode(false), lastEnterTick(0), lastExitTick(0), blockDuration(0), gapDuration(0), prevBlockDuration(0), prevGapDuration(0), newDataReady(false), onDetectCallback(nullptr), isDetected(false) {}

void Sensor::setCallback(void (*cb)()) {
    onDetectCallback = cb;
}

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
        
        // Đọc trạng thái cảm biến PNP
        bool isPinHigh = (port->IDR & pin) != 0;
        
        if (isDiagnosticMode) {
            // Dùng State Machine và Debounce để chống nhiễu tuyệt đối cho Logic Analyzer
            static bool lastDiagState = false;
            static uint32_t lastDiagChangeTick = 0;
            
            if (currentTick - lastDiagChangeTick > 20) { // Lọc dội (Bounce)
                if (isPinHigh != lastDiagState) {
                    if (isPinHigh) { 
                        // Cạnh lên (Mức CAO): Bắt đầu che
                        if (lastExitTick > 0) {
                            prevGapDuration = gapDuration;
                            gapDuration = currentTick - lastExitTick; 
                        }
                        lastEnterTick = currentTick;
                    } else {
                        // Cạnh xuống (Mức THẤP): Rút tay ra
                        if (lastEnterTick > 0) {
                            prevBlockDuration = blockDuration;
                            blockDuration = currentTick - lastEnterTick; 
                            newDataReady = true; // Rút tay ra mới gửi CAN
                        }
                        lastExitTick = currentTick;
                    }
                    lastDiagState = isPinHigh;
                    lastDiagChangeTick = currentTick;
                }
            }
        }
        bool isPinLow = !isPinHigh;
        
        if (isPinLow && (currentTick - lastInterruptTick > 20)) {
            // Kích hoạt Callback
            if (onDetectCallback) onDetectCallback();
            isDetected = true;
            lastInterruptTick = currentTick;
        }
    }
}








