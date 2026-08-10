#ifndef CANBUS_H
#define CANBUS_H

#include "stm32f1xx_hal.h"

class CanBus {
private:
    CAN_HandleTypeDef* hcan;      // Con tr? linh ho?t cho m?i d? án
    CAN_TxHeaderTypeDef TxHeader;
    CAN_RxHeaderTypeDef RxHeader;
    uint32_t TxMailbox;

public:
    // Constructor nh?n b? di?u khi?n CAN sinh ra t? CubeMX
    CanBus(CAN_HandleTypeDef* hcan_ptr);
    
    // Kh?i t?o b? l?c, b?t CAN và ng?t
    void init();
    
    // Hàm g?i d? li?u (Tr? v? true n?u dua vào h?p thu thành công)
    bool sendFrame(uint32_t id, uint8_t* data, uint8_t length);
    
    // Hàm d?c d? li?u t? FIFO (Dùng trong ng?t ho?c vòng l?p)
    bool readFrame(uint32_t* id, uint8_t* data, uint8_t* length);
};

#endif