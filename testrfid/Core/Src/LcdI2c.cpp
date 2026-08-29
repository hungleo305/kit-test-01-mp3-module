#include "LcdI2c.h"

extern "C" void HAL_Delay(uint32_t Delay);

LcdI2c::LcdI2c(I2cBus* i2c_bus, uint8_t addr, uint8_t c, uint8_t r) {
    i2c = i2c_bus;
    address = addr << 1; 
    cols = c;
    rows = r;
    backlightval = LCD_BL;
}

void LcdI2c::init() {
    HAL_Delay(50);
    expanderWrite(backlightval);
    HAL_Delay(1000);

    write4bits(0x03 << 4, 0);
    HAL_Delay(5);
    write4bits(0x03 << 4, 0);
    HAL_Delay(5);
    write4bits(0x03 << 4, 0);
    HAL_Delay(2);
    
    write4bits(0x02 << 4, 0);
    HAL_Delay(2);

    sendCommand(LCD_FUNCTIONSET | 0x08); 
    sendCommand(LCD_DISPLAYCONTROL | 0x04); 
    
    clear();
    sendCommand(LCD_ENTRYMODESET | 0x02); 
}

void LcdI2c::clear() {
    sendCommand(LCD_CLEARDISPLAY);
    HAL_Delay(2);
}

void LcdI2c::setCursor(uint8_t col, uint8_t row) {
    int row_offsets[] = { 0x00, 0x40, 0x14, 0x54 };
    if (row >= rows) { row = rows - 1; }
    sendCommand(LCD_SETDDRAMADDR | (col + row_offsets[row]));
}

void LcdI2c::print(const char* str) {
    while (*str) {
        sendData((uint8_t)(*str));
        str++;
    }
}

void LcdI2c::backlight() {
    backlightval = LCD_BL;
    expanderWrite(0);
}

void LcdI2c::noBacklight() {
    backlightval = 0;
    expanderWrite(0);
}

void LcdI2c::sendCommand(uint8_t cmd) {
    write4bits(cmd & 0xF0, 0);
    write4bits((cmd << 4) & 0xF0, 0);
}

void LcdI2c::sendData(uint8_t data) {
    write4bits(data & 0xF0, LCD_RS);
    write4bits((data << 4) & 0xF0, LCD_RS);
}

void LcdI2c::write4bits(uint8_t value, uint8_t mode) {
    expanderWrite(value | mode | backlightval);
    pulseEnable(value | mode | backlightval);
}

void LcdI2c::expanderWrite(uint8_t data) {
    i2c->WriteByte(address, data);
}

void LcdI2c::pulseEnable(uint8_t data) {
    expanderWrite(data | LCD_EN);
    HAL_Delay(1);
    expanderWrite(data & ~LCD_EN);
    HAL_Delay(1);
}
