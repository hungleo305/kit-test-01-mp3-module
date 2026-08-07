#include "Led7Segment.h"
extern Relay relay;
const uint8_t SEG_ACTUAL_NORMAL[10] = {0x77, 0x11, 0xB6, 0xB3, 0xD1, 0xE3, 0xE7, 0x31, 0xF7, 0xF3};
const uint8_t SEG_ACTUAL_DIGIT3[10] = {0x77, 0x44, 0xB6, 0xE6, 0xC5, 0xE3, 0xF3, 0x46, 0xF7, 0xE7};

#define DATA_PIN    5
#define CLOCK_PIN   7  
#define STROBE_PIN  6  

void Led7Segment::init() {
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
    
    GPIOC->CRL &= ~((0x0F << (DATA_PIN * 4)) | 
                    (0x0F << (CLOCK_PIN * 4)) | 
                      (0x0F << (STROBE_PIN * 4))); 
    GPIOC->CRL |=  ((0x03 << (DATA_PIN * 4)) | 
                    (0x03 << (CLOCK_PIN * 4)) | 
                      (0x03 << (STROBE_PIN * 4))); 
    
    GPIOC->BRR = (1 << DATA_PIN) | (1 << CLOCK_PIN) | (1 << STROBE_PIN);
    
    displayValue = 0;
    colonState = false;
    
    for(int i = 0; i < 4; i++) {
      sendByte(0x00);
      }
    latch();
    GPIOC->BRR = (1 << CLOCK_PIN);
}

void Led7Segment::clockPulse()
{
    GPIOC->BSRR = (1 << CLOCK_PIN);
    __NOP(); __NOP();
    GPIOC->BRR  = (1 << CLOCK_PIN);
    __NOP();
}
void Led7Segment::sendBit(uint8_t bit)
{
    if (bit) {
        GPIOC->BSRR = (1 << DATA_PIN);
    } else {
        GPIOC->BRR  = (1 << DATA_PIN);
    }
    clockPulse();
}

void Led7Segment::sendByte(uint8_t data) {
    for (int8_t i = 7; i >= 0; i--) {
        sendBit((data >> i) & 0x01);
    }
}

void Led7Segment::latch()
{
    GPIOC->BSRR = (1 << STROBE_PIN);
    __NOP(); __NOP(); __NOP(); __NOP();
    __NOP(); __NOP();
    GPIOC->BRR  = (1 << STROBE_PIN);
}

void Led7Segment::setValue(uint16_t val) {
    if (val > 9999) val = 9999;
    displayValue = val;
}

void Led7Segment::setColon(bool state) {
    colonState = state;
}

void Led7Segment::update()
{
    __disable_irq();

    // Gi? STROBE th?p
    GPIOC->BRR = (1 << STROBE_PIN);

    uint8_t digits[4] = {
        (uint8_t)((displayValue / 1000) % 10),
        (uint8_t)((displayValue / 100)  % 10),
        (uint8_t)((displayValue / 10)   % 10),
        (uint8_t)( displayValue % 10)
    };

    for (int i = 0; i < 4; i++)
    {
        uint8_t segment = (i == 2) ? SEG_ACTUAL_DIGIT3[digits[i]] : SEG_ACTUAL_NORMAL[digits[i]];

        if ((i == 1 || i == 2) && colonState) {
            segment |= 0x08;
        }

        // G?i nhanh
        for (int8_t b = 7; b >= 0; b--) {
            if (segment & (1 << b))
                GPIOC->BSRR = (1 << DATA_PIN);
            else
                GPIOC->BRR  = (1 << DATA_PIN);

            clockPulse();
        }
    }
    GPIOC->BSRR = (1 << STROBE_PIN);
    __NOP(); __NOP(); __NOP(); __NOP();
    GPIOC->BRR  = (1 << STROBE_PIN);

    if (relay.isOn()) {
        GPIOC->BSRR = (1 << CLOCK_PIN);
    } else {
        GPIOC->BRR  = (1 << CLOCK_PIN);
    }

    __enable_irq();
}