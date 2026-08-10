#include "CanBus.h"

CanBus::CanBus(CAN_HandleTypeDef* hcan_ptr) {
    hcan = hcan_ptr;
}

void CanBus::init() {
    CAN_FilterTypeDef canFilterConfig;

    // C?u hình b? l?c m? hoàn toàn (Nh?n m?i ID)
    canFilterConfig.FilterBank = 0;
    canFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
    canFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
    canFilterConfig.FilterIdHigh = 0x0000;
    canFilterConfig.FilterIdLow = 0x0000;
    canFilterConfig.FilterMaskIdHigh = 0x0000;
    canFilterConfig.FilterMaskIdLow = 0x0000;
    canFilterConfig.FilterFIFOAssignment = CAN_RX_FIFO0;
    canFilterConfig.FilterActivation = ENABLE;
    canFilterConfig.SlaveStartFilterBank = 14;

    // 1. Áp d?ng b? l?c
    HAL_CAN_ConfigFilter(hcan, &canFilterConfig);
    
    // 2. Kh?i d?ng ngo?i vi CAN
    HAL_CAN_Start(hcan);
    
    // 3. Kích ho?t ng?t nh?n d? li?u (Rx FIFO0)
    HAL_CAN_ActivateNotification(hcan, CAN_IT_RX_FIFO0_MSG_PENDING);
}

bool CanBus::sendFrame(uint32_t id, uint8_t* data, uint8_t length) {
    if (length > 8) return false; // Khung CAN 2.0A ch? t?i da 8 byte

    TxHeader.StdId = id;
    TxHeader.ExtId = 0x00;
    TxHeader.RTR = CAN_RTR_DATA;
    TxHeader.IDE = CAN_ID_STD;
    TxHeader.DLC = length;
    TxHeader.TransmitGlobalTime = DISABLE;

    // Ð?y d? li?u vào Mailbox d? ph?n c?ng t? d?ng g?i
    if (HAL_CAN_AddTxMessage(hcan, &TxHeader, data, &TxMailbox) == HAL_OK) {
        return true;
    }
    return false;
}

bool CanBus::readFrame(uint32_t* id, uint8_t* data, uint8_t* length) {
    // Ki?m tra xem FIFO có d? li?u không
    if (HAL_CAN_GetRxFifoFillLevel(hcan, CAN_RX_FIFO0) > 0) {
        if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &RxHeader, data) == HAL_OK) {
            *id = RxHeader.StdId;
            *length = RxHeader.DLC;
            return true;
        }
    }
    return false;
}