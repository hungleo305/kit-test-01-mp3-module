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
    
    void init();
    
    bool sendFrame(uint32_t id, uint8_t* data, uint8_t length);

    bool readFrame(uint32_t* id, uint8_t* data, uint8_t* length);
};

#endif