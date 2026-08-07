/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    user_diskio.c
 * @brief   Software SPI Driver (T?i uu d? tr? cho th? 2GB)
 ******************************************************************************
 */
/* USER CODE END Header */

#include <string.h>
#include "ff_gen_drv.h"
#include "stm32f1xx_hal.h"

#define SD_CS_PORT GPIOC
#define SD_CS_PIN  GPIO_PIN_8
#define SD_CLK_PORT  GPIOA
#define SD_CLK_PIN   GPIO_PIN_5
#define SD_MISO_PORT GPIOA
#define SD_MISO_PIN  GPIO_PIN_6
#define SD_MOSI_PORT GPIOA
#define SD_MOSI_PIN  GPIO_PIN_7

#define SD_CS_LOW()   HAL_GPIO_WritePin(SD_CS_PORT, SD_CS_PIN, GPIO_PIN_RESET)
#define SD_CS_HIGH()  HAL_GPIO_WritePin(SD_CS_PORT, SD_CS_PIN, GPIO_PIN_SET)
#define SD_CLK_LOW()  HAL_GPIO_WritePin(SD_CLK_PORT, SD_CLK_PIN, GPIO_PIN_RESET)
#define SD_CLK_HIGH() HAL_GPIO_WritePin(SD_CLK_PORT, SD_CLK_PIN, GPIO_PIN_SET)
#define SD_MOSI_LOW() HAL_GPIO_WritePin(SD_MOSI_PORT, SD_MOSI_PIN, GPIO_PIN_RESET)
#define SD_MOSI_HIGH() HAL_GPIO_WritePin(SD_MOSI_PORT, SD_MOSI_PIN, GPIO_PIN_SET)
#define SD_MISO_READ() HAL_GPIO_ReadPin(SD_MISO_PORT, SD_MISO_PIN)

static volatile DSTATUS Stat = STA_NOINIT;

static void SD_GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();
    
    GPIO_InitStruct.Pin = SD_CLK_PIN | SD_MOSI_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    GPIO_InitStruct.Pin = SD_MISO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

static void SPI_Delay(void) {
    for(volatile int i=0; i<10; i++); 
}

static void SPI_TxByte(uint8_t data) {
    for (uint8_t i = 0; i < 8; i++) {
        if (data & 0x80) SD_MOSI_HIGH();
        else SD_MOSI_LOW();
        data <<= 1;
        SD_CLK_HIGH(); 
        SPI_Delay();
        SD_CLK_LOW();  
        SPI_Delay();
    }
}

static uint8_t SPI_RxByte(void) {
    uint8_t data = 0;
    SD_MOSI_HIGH(); 
    for (uint8_t i = 0; i < 8; i++) {
        data <<= 1;
        SD_CLK_HIGH(); 
        SPI_Delay();
        if (SD_MISO_READ()) data |= 0x01;
        SD_CLK_LOW();
        SPI_Delay();
    }
    return data;
}

static uint8_t SD_WaitReady(void) {
    uint8_t res;
    uint32_t tickstart = HAL_GetTick();
    do {
        res = SPI_RxByte();
    } while (res != 0xFF && (HAL_GetTick() - tickstart) < 500);
    return res;
}

static uint8_t SD_SendCmd(uint8_t cmd, uint32_t arg) {
    uint8_t res, n;
    SPI_RxByte(); 
    SD_CS_LOW();
    SD_WaitReady();

    SPI_TxByte(0x40 | cmd);
    SPI_TxByte((uint8_t)(arg >> 24));
    SPI_TxByte((uint8_t)(arg >> 16));
    SPI_TxByte((uint8_t)(arg >> 8));
    SPI_TxByte((uint8_t)arg);

    n = 0x01; 
    if (cmd == 0) n = 0x95; 
    if (cmd == 8) n = 0x87; 
    SPI_TxByte(n);

    n = 10;
    do { res = SPI_RxByte(); } while ((res & 0x80) && --n);
    return res;
}

DSTATUS USER_initialize (BYTE pdrv);
DSTATUS USER_status (BYTE pdrv);
DRESULT USER_read (BYTE pdrv, BYTE *buff, DWORD sector, UINT count);
#if _USE_WRITE == 1
  DRESULT USER_write (BYTE pdrv, const BYTE *buff, DWORD sector, UINT count);
#endif 
#if _USE_IOCTL == 1
  DRESULT USER_ioctl (BYTE pdrv, BYTE cmd, void *buff);
#endif 

Diskio_drvTypeDef  USER_Driver =
{
  USER_initialize,
  USER_status,
  USER_read,
#if  _USE_WRITE
  USER_write,
#endif  
#if  _USE_IOCTL == 1
  USER_ioctl,
#endif 
};

DSTATUS USER_initialize (BYTE pdrv) {
    uint16_t i;
    SD_GPIO_Init();
    
    SD_CS_HIGH();
    HAL_Delay(100); 
    
    for (i = 0; i < 100; i++) SPI_TxByte(0xFF);
    
    if (SD_SendCmd(0, 0) == 1) {
        uint32_t tickstart = HAL_GetTick();
        do {
            SD_SendCmd(55, 0); 
            if (SD_SendCmd(41, 1UL << 30) == 0) { 
                Stat &= ~STA_NOINIT; 
                break;
            }
        } while ((HAL_GetTick() - tickstart) < 1000); 
    }
    SD_CS_HIGH();
    SPI_RxByte();
    return Stat;
}

DSTATUS USER_status (BYTE pdrv) { return Stat; }

DRESULT USER_read (BYTE pdrv, BYTE *buff, DWORD sector, UINT count) {
    DRESULT res = RES_ERROR;
    if (Stat & STA_NOINIT) return RES_NOTRDY;
    
    sector *= 512; 
    
    while (count > 0) {
        if (SD_SendCmd(17, sector) == 0) {
            uint32_t tickstart = HAL_GetTick();
            while (SPI_RxByte() != 0xFE) {
                if ((HAL_GetTick() - tickstart) > 500) goto read_fail;
            }
            for (int i = 0; i < 512; i++) buff[i] = SPI_RxByte();
            SPI_RxByte(); SPI_RxByte(); 
            sector += 512; buff += 512; count--;
            res = RES_OK;
        } else break;
    }
read_fail:
    SD_CS_HIGH();
    SPI_RxByte();
    return res;
}

#if _USE_WRITE == 1
DRESULT USER_write (BYTE pdrv, const BYTE *buff, DWORD sector, UINT count) {
    DRESULT res = RES_ERROR;
    if (Stat & STA_NOINIT) return RES_NOTRDY;

    sector *= 512;
    
    while (count > 0) {
        if (SD_SendCmd(24, sector) == 0) {
            SPI_TxByte(0xFE);
            for (int i = 0; i < 512; i++) SPI_TxByte(buff[i]);
            SPI_TxByte(0xFF); SPI_TxByte(0xFF);
            if ((SPI_RxByte() & 0x1F) == 0x05) {
                SD_WaitReady();
                sector += 512; buff += 512; count--;
                res = RES_OK;
            } else break;
        } else break;
    }
    SD_CS_HIGH();
    SPI_RxByte();
    return res;
}
#endif

#if _USE_IOCTL == 1
DRESULT USER_ioctl (BYTE pdrv, BYTE cmd, void *buff) {
    DRESULT res = RES_ERROR;
    if (Stat & STA_NOINIT) return RES_NOTRDY;
    switch (cmd) {
        case CTRL_SYNC: SD_CS_LOW(); if (SD_WaitReady() == 0xFF) res = RES_OK; SD_CS_HIGH(); break;
        case GET_SECTOR_SIZE: *(WORD*)buff = 512; res = RES_OK; break;
        case GET_BLOCK_SIZE: *(DWORD*)buff = 1; res = RES_OK; break;
        default: res = RES_PARERR;
    }
    return res;
}
#endif