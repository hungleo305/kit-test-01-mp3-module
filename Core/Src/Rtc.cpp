#include "Rtc.h"

uint8_t Rtc::decToBcd(uint8_t val) {
    return ((val / 10) << 4) | (val % 10);
}

uint8_t Rtc::bcdToDec(uint8_t val) {
    return ((val >> 4) * 10) + (val & 0x0F);
}

Rtc::Rtc(I2cBus* i2c_bus) : i2c_bus(i2c_bus) {}

void Rtc::init() {
    // Không cần cấu hình Clock và GPIO phức tạp ở đây nữa
    // Tất cả đã được xử lý bên trong i2c_bus->Init()
    i2c_bus->Init();
}

void Rtc::setTime(uint8_t hour, uint8_t minute, uint8_t second) {
    i2c_bus->WriteMem(0xD0, 0x00, decToBcd(second) & 0x7F); 
    i2c_bus->WriteMem(0xD0, 0x01, decToBcd(minute));
    i2c_bus->WriteMem(0xD0, 0x02, decToBcd(hour) & 0x3F);      
}

void Rtc::getTime(uint8_t &hour, uint8_t &minute, uint8_t &second) {
    second = bcdToDec(i2c_bus->ReadMem(0xD0, 0x00) & 0x7F);
    minute = bcdToDec(i2c_bus->ReadMem(0xD0, 0x01));
    hour   = bcdToDec(i2c_bus->ReadMem(0xD0, 0x02) & 0x3F);
}

bool Rtc::isTimeLost() {
    // Đọc thử vài lần để tránh nhiễu I2C lúc khởi động
    for (int i = 0; i < 3; i++) {
        uint8_t secReg = i2c_bus->ReadMem(0xD0, 0x00);
        // Nếu đọc được bit 7 == 0 (Clock đang chạy), thì kết luận là không lỗi
        if ((secReg & 0x80) == 0) {
            return false;
        }
        HAL_Delay(5);
    }
    // Nếu cả 3 lần đọc đều thấy bit 7 == 1 (Clock Halt), chắc chắn mất thời gian
    return true;
}

void Rtc::setDate(uint8_t dayOfWeek, uint8_t date, uint8_t month, uint8_t year) {
    i2c_bus->WriteMem(0xD0, 0x03, decToBcd(dayOfWeek));
    i2c_bus->WriteMem(0xD0, 0x04, decToBcd(date));
    i2c_bus->WriteMem(0xD0, 0x05, decToBcd(month));
    i2c_bus->WriteMem(0xD0, 0x06, decToBcd(year));
}

void Rtc::getDate(uint8_t &dayOfWeek, uint8_t &date, uint8_t &month, uint8_t &year) {
    dayOfWeek = bcdToDec(i2c_bus->ReadMem(0xD0, 0x03));
    date      = bcdToDec(i2c_bus->ReadMem(0xD0, 0x04));
    month     = bcdToDec(i2c_bus->ReadMem(0xD0, 0x05));
    year      = bcdToDec(i2c_bus->ReadMem(0xD0, 0x06));
}
