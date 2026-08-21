#include "SystemTimer.h"

uint32_t SystemTimer::getTick() {
    return HAL_GetTick();
}

void SystemTimer::delay_ms(uint32_t ms) {
    HAL_Delay(ms);
}