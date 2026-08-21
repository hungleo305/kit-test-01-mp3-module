#include "CanBus.h"

CanBus::CanBus(CAN_HandleTypeDef* hcan_ptr) {
    hcan = hcan_ptr;
}

void CanBus::init() {
    CAN_FilterTypeDef canFilterConfig;
    
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

    HAL_CAN_ConfigFilter(hcan, &canFilterConfig);
    
    HAL_CAN_Start(hcan);
    
    HAL_CAN_ActivateNotification(hcan, CAN_IT_RX_FIFO0_MSG_PENDING);
}

bool CanBus::sendFrame(uint32_t id, uint8_t* data, uint8_t length) {
    if (length > 8) return false;

    TxHeader.StdId = id;
    TxHeader.ExtId = 0x00;
    TxHeader.RTR = CAN_RTR_DATA;
    TxHeader.IDE = CAN_ID_STD;
    TxHeader.DLC = length;
    TxHeader.TransmitGlobalTime = DISABLE;

    if (HAL_CAN_AddTxMessage(hcan, &TxHeader, data, &TxMailbox) == HAL_OK) {
        return true;
    }
    return false;
}

bool CanBus::readFrame(uint32_t* id, uint8_t* data, uint8_t* length) {
    if (HAL_CAN_GetRxFifoFillLevel(hcan, CAN_RX_FIFO0) > 0) {
        if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &RxHeader, data) == HAL_OK) {
            *id = RxHeader.StdId;
            *length = RxHeader.DLC;
            return true;
        }
    }
    return false;
}