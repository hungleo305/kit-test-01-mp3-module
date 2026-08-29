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
    CanBus(CAN_HandleTypeDef* hcan_ptr);
    
    void init(uint32_t filterId = 0x0000, uint32_t filterMask = 0x0000);
    
    bool sendFrame(uint32_t id, uint8_t* data, uint8_t length);

    bool readFrame(uint32_t* id, uint8_t* data, uint8_t* length);
};

#endif