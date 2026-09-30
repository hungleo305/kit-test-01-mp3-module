#ifndef CANBUS_H
#define CANBUS_H

#include "stm32f1xx_hal.h"

class CanBus {
private:
    CAN_HandleTypeDef* hcan;    
    CAN_TxHeaderTypeDef TxHeader;
    CAN_RxHeaderTypeDef RxHeader;
    uint32_t TxMailbox;

public:
    uint32_t txSuccessCount;
    CanBus(CAN_HandleTypeDef* hcan_ptr);
    
    // Cấu hình bộ lọc CAN (Filter) để chỉ nhận những tin nhắn có ID mong muốn. Gọi 1 lần ở setup.
    // Mặc định (0x0000, 0x0000) nghĩa là nhận tất cả mọi tin nhắn.
    void init(uint32_t filterId = 0x0000, uint32_t filterMask = 0x0000);
    
    // Gửi một gói tin lên mạng CAN.
    // id: Địa chỉ của gói tin (VD: 0x123).
    // data: Mảng dữ liệu cần gửi.
    // length: Độ dài dữ liệu (tối đa 8 byte).
    // Trả về true nếu đẩy vào hộp thư gửi (Mailbox) thành công.
    bool sendFrame(uint32_t id, uint8_t* data, uint8_t length);

    // Đọc gói tin từ hộp thư nhận (FIFO). Thường được gọi trong ngắt (Interrupt).
    // id: Con trỏ hứng địa chỉ của gói tin vừa nhận.
    // data: Con trỏ mảng hứng dữ liệu.
    // length: Con trỏ hứng độ dài dữ liệu vừa nhận được.
    // Trả về true nếu có tin nhắn mới và đọc thành công.
    bool readFrame(uint32_t* id, uint8_t* data, uint8_t* length);
};

#endif