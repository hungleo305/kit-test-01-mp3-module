#include "Led7Segment.h"

const uint8_t BANG_MA_LED_THUONG[10] = {0x77, 0x11, 0xB6, 0xB3, 0xD1, 0xE3, 0xE7, 0x31, 0xF7, 0xF3};

const uint8_t BANG_MA_LED_SO3[10] = {0x77, 0x44, 0xB6, 0xE6, 0xC5, 0xE3, 0xF3, 0x46, 0xF7, 0xE7};


Led7Segment::Led7Segment(GPIO_TypeDef* dataPort, uint16_t dataPin, 
                         GPIO_TypeDef* clockPort, uint16_t clockPin, 
                         GPIO_TypeDef* strobePort, uint16_t strobePin, 
                         bool (*getClockState)()) 
    : dataPort(dataPort), dataPin(dataPin), 
      clockPort(clockPort), clockPin(clockPin), 
      strobePort(strobePort), strobePin(strobePin), 
      getClockState(getClockState) {}

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
            ports[i]->CRL |= (0x3 << (pinIndex * 4)); 
        } else {
            ports[i]->CRH &= ~(0xF << ((pinIndex - 8) * 4));
            ports[i]->CRH |= (0x3 << ((pinIndex - 8) * 4));
        }
        ports[i]->BRR = pins[i];
    }
    
    displayValue = 0;
    colonState = false;
    cachedClockState = false;
    
    for(int i = 0; i < 4; i++) {
      sendByte(0x00);
    }
    latch();
    clockPort->BRR = clockPin;
}

void Led7Segment::clockPulse()
{
    if (cachedClockState) {
        // Relay is ON (pin is HIGH).
        clockPort->BRR = clockPin;              
        for(volatile int d=0; d<5; d++);           
        clockPort->BSRR = clockPin;                
        for(volatile int d=0; d<5; d++);           
    } else {
        // Relay is OFF (pin is LOW).
        clockPort->BSRR = clockPin;                
        for(volatile int d=0; d<5; d++);            
        clockPort->BRR = clockPin;                  
        for(volatile int d=0; d<5; d++);            
    }
}

void Led7Segment::sendBit(uint8_t bit)
{
    if (bit) {
        dataPort->BSRR = dataPin;
    } else {
        dataPort->BRR  = dataPin;
    }
    __NOP(); __NOP();
    clockPulse();
}

void Led7Segment::sendByte(uint8_t data) {
    for (int8_t i = 7; i >= 0; i--) {
        sendBit((data >> i) & 0x01);
    }
}

void Led7Segment::latch()
{
    strobePort->BSRR = strobePin;
    __NOP(); __NOP(); __NOP(); __NOP();
    __NOP(); __NOP();
    strobePort->BRR  = strobePin;
}

void Led7Segment::setValue(uint16_t val) {
    if (val > 9999) val = 9999;
    displayValue = val;
}

void Led7Segment::setColon(bool state) {
    colonState = state;
}

void Led7Segment::update(bool isTimeMode)
{
    __disable_irq();

    cachedClockState = (getClockState != nullptr) ? getClockState() : false;

    // Turn OFF screen to hide shifting (OE HIGH)
    strobePort->BSRR = strobePin; 

    uint8_t digits[4] = {
        (uint8_t)((displayValue / 1000) % 10),
        (uint8_t)((displayValue / 100)  % 10),
        (uint8_t)((displayValue / 10)   % 10),
        (uint8_t)( displayValue % 10)
    };
    //dem den dau hien thi den do
      bool isLeadingZero = !isTimeMode;
      for (int i =0 ; i<4;i ++){
        if (digits[i] != 0 || i ==3){
          isLeadingZero = false;
        }
        
        uint8_t segment;
        if(isLeadingZero){
          segment = 0x00;
        }else{
          segment = (i==2) ? BANG_MA_LED_SO3[digits[i]] : BANG_MA_LED_THUONG[digits[i]];
        }
        
        if ((i == 1 || i == 2) && colonState) {
            segment |= 0x08;
        }

        for (int8_t b = 7; b >= 0; b--) {
            if (segment & (1 << b))
                dataPort->BSRR = dataPin;
            else
                dataPort->BRR  = dataPin;
            
            __NOP(); __NOP(); 
            clockPulse();
        }
    }
    
    // Turn ON screen to show new data (OE LOW)
    strobePort->BRR = strobePin; 
    
    // Set Data LOW just in case
    dataPort->BRR = dataPin;

    if (cachedClockState) {
        clockPort->BSRR = clockPin;
    } else {
        clockPort->BRR  = clockPin;
    }

    __enable_irq();
}

void Led7Segment::blank() {
    __disable_irq();
    
    cachedClockState = (getClockState != nullptr) ? getClockState() : false;
    
    strobePort->BSRR = strobePin; // Turn OFF screen
    for(int i = 0; i < 4; i++) {
      sendByte(0x00);
    }
    
    strobePort->BRR = strobePin; // Turn ON screen
    
    dataPort->BRR = dataPin;
    if (cachedClockState) {
        clockPort->BSRR = clockPin;
    } else {
        clockPort->BRR  = clockPin;
    }
    __enable_irq();
}

void Led7Segment::restore() {
    update();
}

void Led7Segment::turnOff() {
    strobePort->BSRR = strobePin;
}

void Led7Segment::turnOn() {
    strobePort->BRR = strobePin;
}

void Led7Segment::showDashes() {
    __disable_irq();
    cachedClockState = (getClockState != nullptr) ? getClockState() : false;
    strobePort->BSRR = strobePin; // Turn OFF screen
    
    // Segment 'g' (dấu gạch ngang) tương ứng với bit 7 (0x80) cho cả chip chuẩn và SO3
    for(int i = 0; i < 4; i++) {
        uint8_t segment = 0x80;
        for (int8_t b = 7; b >= 0; b--) {
            if (segment & (1 << b)) dataPort->BSRR = dataPin;
            else dataPort->BRR = dataPin;
            __NOP(); __NOP(); clockPulse();
        }
    }
    
    strobePort->BRR = strobePin; // Turn ON screen
    dataPort->BRR = dataPin;
    if (cachedClockState) clockPort->BSRR = clockPin;
    else clockPort->BRR  = clockPin;
    __enable_irq();
}