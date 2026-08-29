#ifndef LCD_I2C_H
#define LCD_I2C_H

#include "I2cBus.h"

// PCF8574 LCD Adapter Pinout
#define LCD_RS 0x01 // Register Select: 0=Command, 1=Data
#define LCD_RW 0x02 // Read/Write: 0=Write, 1=Read
#define LCD_EN 0x04 // Enable
#define LCD_BL 0x08 // Backlight

// LCD Commands
#define LCD_CLEARDISPLAY 0x01
#define LCD_RETURNHOME 0x02
#define LCD_ENTRYMODESET 0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_CURSORSHIFT 0x10
#define LCD_FUNCTIONSET 0x20
#define LCD_SETCGRAMADDR 0x40
#define LCD_SETDDRAMADDR 0x80

class LcdI2c {
private:
    I2cBus* i2c;
    uint8_t address;
    uint8_t cols;
    uint8_t rows;
    uint8_t backlightval;

    void sendCommand(uint8_t cmd);
    void sendData(uint8_t data);
    void write4bits(uint8_t value, uint8_t mode);
    void expanderWrite(uint8_t data);
    void pulseEnable(uint8_t data);

public:
    LcdI2c(I2cBus* i2c_bus, uint8_t addr = 0x27, uint8_t cols = 16, uint8_t rows = 2);
    
    void init();
    void clear();
    void setCursor(uint8_t col, uint8_t row);
    void print(const char* str);
    void backlight();
    void noBacklight();
};

#endif // LCD_I2C_H
