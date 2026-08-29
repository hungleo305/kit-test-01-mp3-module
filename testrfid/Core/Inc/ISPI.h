#ifndef ISPI_H
#define ISPI_H

#include <stdint.h>

class ISPI {
public:
    virtual void Init() = 0;
    virtual uint8_t Transfer(uint8_t data) = 0;
    virtual void TransferBuffer(const uint8_t* tx_data, uint8_t* rx_data, uint16_t size) = 0;
    virtual void SetSpeed(uint32_t speed_hz) = 0;
    virtual void SetPrescaler(uint32_t prescaler) = 0;
    virtual ~ISPI() {}
};

#endif
