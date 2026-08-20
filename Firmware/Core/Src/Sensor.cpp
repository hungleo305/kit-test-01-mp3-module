#include "Sensor.h"
#include "SystemTimer.h"

volatile uint16_t Sensor::productCount = 0;
volatile bool Sensor::isDetected = false;
uint32_t Sensor::lastInterruptTick = 0;

volatile bool Sensor::isDiagnosticMode = false;
volatile uint32_t Sensor::lastEnterTick = 0;
volatile uint32_t Sensor::lastExitTick = 0;
volatile uint32_t Sensor::blockDuration = 0;
volatile uint32_t Sensor::gapDuration = 0;
volatile bool Sensor::newDataReady = false;

void Sensor::enableDiagnosticMode(bool enable) {
    isDiagnosticMode = enable;
    if (enable) {
        EXTI->RTSR |= (1 << 13); // Bật thêm ngắt cạnh lên (Rút tay ra)
    } else {
        EXTI->RTSR &= ~(1 << 13); // Tắt ngắt cạnh lên, trở về bình thường
    }
}

bool Sensor::getDiagnosticData(uint32_t& outBlockDuration, uint32_t& outGapDuration) {
    if (newDataReady) {
        outBlockDuration = blockDuration;
        outGapDuration = gapDuration;
        newDataReady = false;
        return true;
    }
    return false;
}

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
        
        // Đọc trạng thái chân PC13 (0 = bị che, 1 = không bị che) -- Đang giả định NPN
        // Nhưng thực tế là PNP (1 = Bị che, 0 = Hở)
        bool isPinHigh = (GPIOC->IDR & (1 << 13)) != 0;
        
        if (isDiagnosticMode) {
            // Dùng State Machine và Debounce để chống nhiễu tuyệt đối cho Logic Analyzer
            static bool lastDiagState = false;
            static uint32_t lastDiagChangeTick = 0;
            
            if (currentTick - lastDiagChangeTick > 20) { // Lọc dội phím (Bounce)
                if (isPinHigh != lastDiagState) {
                    if (!isPinHigh) { 
                        // Cạnh xuống: Bắt đầu che (Cảm biến báo 0)
                        if (lastExitTick > 0) {
                            gapDuration = currentTick - lastExitTick; 
                        }
                        lastEnterTick = currentTick;
                    } else {
                        // Cạnh lên: Rút tay ra (Cảm biến báo 1)
                        if (lastEnterTick > 0) {
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
        
        //Giảm thời gian chống rung xuống 20ms (trước đây là 200ms làm lọt số)
        // Lưu ý: Code này đếm sản phẩm khi RÚT TAY RA (Cạnh lên, isPinHigh = 1)
        if (isPinHigh && (currentTick - lastInterruptTick > 20)) {
            //Ghi giá trị đếm mới vào thanh ghi BKP->DR1 để chống mất điện
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
