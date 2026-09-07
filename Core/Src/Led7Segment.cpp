#include "Led7Segment.h"

#define SEG_A  (1U << 0)
#define SEG_B  (1U << 1)
#define SEG_C  (1U << 2)
#define SEG_D  (1U << 3)
#define SEG_E  (1U << 4)
#define SEG_F  (1U << 5)
#define SEG_G  (1U << 6)
#define SEG_DP (1U << 7)

// 1. CẬP NHẬT HÀM KHỞI TẠO (Thêm isOE(false))
Led7Segment::Led7Segment(GPIO_TypeDef* dPort, uint16_t dPin, 
                         GPIO_TypeDef* clkPort, uint16_t clkPin, 
                         GPIO_TypeDef* strPort, uint16_t strPin,
                         uint8_t digits) 
    : dataPort(dPort), dataPin(dPin), 
      clockPort(clkPort), clockPin(clkPin), 
      strobePort(strPort), strobePin(strPin),
      numDigits(digits), revMask(0), isInverted(false), isOE(false) 
{
    for (uint8_t i = 0; i < 8; i++) segMap[i] = i;
}

void Led7Segment::setSegmentMap(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t e, uint8_t f, uint8_t g, uint8_t dp) {
    segMap[0] = a; segMap[1] = b; segMap[2] = c; segMap[3] = d;
    segMap[4] = e; segMap[5] = f; segMap[6] = g; segMap[7] = dp;
}
void Led7Segment::setInverted(bool state) { isInverted = state; }
void Led7Segment::setReverseMask(uint32_t mask) { revMask = mask; }

// 2. THÊM 2 HÀM CẤU HÌNH OE
void Led7Segment::setStrobeOE(bool state) { isOE = state; }
void Led7Segment::hide() {
    if (isOE) strobePort->BSRR = strobePin; // OE mức CAO là tắt màn hình
}

void Led7Segment::init() {
    if (dataPort == GPIOA || clockPort == GPIOA || strobePort == GPIOA) RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
    if (dataPort == GPIOB || clockPort == GPIOB || strobePort == GPIOB) RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
    if (dataPort == GPIOC || clockPort == GPIOC || strobePort == GPIOC) RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
    if (dataPort == GPIOD || clockPort == GPIOD || strobePort == GPIOD) RCC->APB2ENR |= RCC_APB2ENR_IOPDEN;
    
    GPIO_TypeDef* ports[] = {dataPort, clockPort, strobePort};
    uint16_t pins[] = {dataPin, clockPin, strobePin};

    for(int i = 0; i < 3; i++) {
        uint32_t pinIndex = 0;
        while (!(pins[i] & (1 << pinIndex))) pinIndex++;

        if (pinIndex < 8) {
            ports[i]->CRL &= ~(0xF << (pinIndex * 4));
            ports[i]->CRL |=  (0x3 << (pinIndex * 4)); 
        } else {
            ports[i]->CRH &= ~(0xF << ((pinIndex - 8) * 4));
            ports[i]->CRH |=  (0x3 << ((pinIndex - 8) * 4));
        }
        ports[i]->BRR = pins[i]; 
    }
    blank(); 
}

uint8_t Led7Segment::getLogicalDigit(uint8_t digit) {
    const uint8_t LOGICAL_DIGITS[10] = { 0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F };
    if (digit > 9) return 0x00;
    return LOGICAL_DIGITS[digit];
}

uint8_t Led7Segment::mapLogicalToPhysical(uint8_t logicalSeg, bool isReversed) {
    if (isReversed) {
        uint8_t rev = 0;
        if (logicalSeg & SEG_A)  rev |= SEG_D;
        if (logicalSeg & SEG_B)  rev |= SEG_E;
        if (logicalSeg & SEG_C)  rev |= SEG_F;
        if (logicalSeg & SEG_D)  rev |= SEG_A;
        if (logicalSeg & SEG_E)  rev |= SEG_B;
        if (logicalSeg & SEG_F)  rev |= SEG_C;
        if (logicalSeg & SEG_G)  rev |= SEG_G;
        if (logicalSeg & SEG_DP) rev |= SEG_DP;
        logicalSeg = rev;
    }
    uint8_t physical = 0;
    for (uint8_t i = 0; i < 8; i++) {
        if (logicalSeg & (1U << i)) physical |= (1U << segMap[i]);
    }
    return physical;
}

void Led7Segment::clockPulse() {
    bool isHigh = (clockPort->ODR & clockPin) != 0;
    if (isHigh) {
        clockPort->BRR = clockPin;
        for(volatile int d=0; d<5; d++); 
        clockPort->BSRR = clockPin;
        for(volatile int d=0; d<5; d++);
    } else {
        clockPort->BSRR = clockPin;                
        for(volatile int d=0; d<5; d++);            
        clockPort->BRR = clockPin;                  
        for(volatile int d=0; d<5; d++);
    }
}

void Led7Segment::sendBit(uint8_t bit) {
    if (bit) dataPort->BSRR = dataPin;
    else     dataPort->BRR  = dataPin;
    clockPulse();
}

void Led7Segment::sendByte(uint8_t data) {
    for (int8_t i = 7; i >= 0; i--) sendBit((data >> i) & 0x01);
}

void Led7Segment::latch() {
    strobePort->BSRR = strobePin;
    __NOP(); __NOP(); __NOP();
    strobePort->BRR = strobePin;
}

void Led7Segment::update(uint32_t value, bool padZeroes, uint32_t dotMask) {
    clearBuffer();
    setNumber(value, 0, numDigits, padZeroes);
    for (int i = 0; i < numDigits; i++) {
        if (dotMask & (1 << i)) {
            displayBuffer[i] |= SEG_DP;
            dotBuffer |= (1 << i);
        }
    }
    refresh();
}

void Led7Segment::updateSplit(uint32_t rightVal, uint8_t rightDigits, bool rightPad, uint32_t leftVal, uint8_t leftDigits, bool leftPad, uint32_t dotMask) {
    clearBuffer();
    if (rightDigits > 0) setNumber(rightVal, 0, rightDigits, rightPad);
    if (leftDigits > 0) setNumber(leftVal, rightDigits, leftDigits, leftPad);
    for (int i = 0; i < numDigits; i++) {
        if (dotMask & (1 << i)) {
            displayBuffer[i] |= SEG_DP;
            dotBuffer |= (1 << i);
        }
    }
    refresh();
}

void Led7Segment::clearBuffer() {
    for (int i = 0; i < 16; i++) displayBuffer[i] = 0x00;
    dotBuffer = 0;
}

void Led7Segment::setDigit(uint8_t index, uint8_t value, bool showDot) {
    if (index >= 16) return;
    uint8_t logicalSeg = 0x00;
    if (value == 0xFF) logicalSeg = 0x00;
    else if (value == 0xFE) logicalSeg = 0x40;
    else logicalSeg = getLogicalDigit(value);
    
    displayBuffer[index] = logicalSeg;
    if (showDot) dotBuffer |= (1 << index);
    else dotBuffer &= ~(1 << index);
}

void Led7Segment::setRaw(uint8_t index, uint8_t rawHex) {
    if (index >= 16) return;
    displayBuffer[index] = rawHex;
}

void Led7Segment::setNumber(uint32_t number, uint8_t startIndex, uint8_t length, bool padZeroes) {
    uint32_t tempVal = number;
    for (int i = 0; i < length; i++) {
        uint8_t digit = tempVal % 10;
        tempVal /= 10;
        if (!padZeroes && tempVal == 0 && digit == 0 && i > 0) setDigit(startIndex + i, 0xFF, false);
        else setDigit(startIndex + i, digit, false);
    }
}

// 3. SỬA CHUẨN CÁC HÀM REFRESH/BLANK ĐỂ HỖ TRỢ CHUẨN OE
void Led7Segment::refresh() {
    __disable_irq(); 
    if (isOE) strobePort->BSRR = strobePin; // Mạch VNATECH -> Tắt màn hình
    else      strobePort->BRR = strobePin;  // Mạch dp2 -> Khóa chốt
    
    for (int i = numDigits - 1; i >= 0; i--) {
        uint8_t logicalSeg = displayBuffer[i];
        if (dotBuffer & (1 << i)) logicalSeg |= SEG_DP;
        bool isRev = (revMask & (1 << i)) != 0;
        uint8_t physByte = mapLogicalToPhysical(logicalSeg, isRev);
        if (isInverted) physByte = ~physByte; 
        sendByte(physByte);
    }
    
    if (isOE) strobePort->BRR = strobePin;  // Mạch VNATECH -> Bật lại màn hình
    else      latch();                      // Mạch dp2 -> Mở chốt xả data
    
    dataPort->BRR = dataPin;
    __enable_irq();
}

void Led7Segment::blank() {
    __disable_irq();
    if (isOE) strobePort->BSRR = strobePin; 
    else      strobePort->BRR = strobePin; 
    
    for(int i = 0; i < numDigits; i++) sendByte(isInverted ? 0xFF : 0x00);
    
    if (isOE) strobePort->BRR = strobePin; 
    else      latch(); 
    
    dataPort->BRR = dataPin;
    __enable_irq();
}

void Led7Segment::showDashes() {
    __disable_irq();
    if (isOE) strobePort->BSRR = strobePin; 
    else      strobePort->BRR = strobePin; 
    
    for(int i = numDigits - 1; i >= 0; i--) {
        bool isRev = (revMask & (1 << i)) != 0;
        uint8_t physByte = mapLogicalToPhysical(SEG_G, isRev); 
        if (isInverted) physByte = ~physByte; 
        sendByte(physByte);
    }
    
    if (isOE) strobePort->BRR = strobePin; 
    else      latch(); 
    
    dataPort->BRR = dataPin;
    __enable_irq();
}