/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  * @author         : Chu Manh Hung    
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "fatfs.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdio.h>
#include "stm32f1xx_hal.h"
#include "Led.h"
#include "Buzzer.h"
#include "Sensor.h"
#include "Led7Segment.h"
#include "Rtc.h"
#include "Rfid.h"
#include "Relay.h"
#include "Mp3.h"
#include "ff.h"
#include "CanBus.h"
#include "SpiBus.h"
#include "I2cBus.h"
#include "user_diskio.h"
#include "SdCard.h"

extern char USERPath[4];
extern Diskio_drvTypeDef USER_Driver;
extern uint8_t FATFS_UnLinkDriver(char *path);
extern uint8_t FATFS_LinkDriver(Diskio_drvTypeDef *drv, char *path);
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
CAN_HandleTypeDef hcan;

Led ledOb(GPIOB, GPIO_PIN_2);
Buzzer buzzer(GPIOC, GPIO_PIN_15);
Sensor cb(GPIOC, GPIO_PIN_13);
Relay relay(GPIOC, GPIO_PIN_7);

volatile bool rtcErrorFlag = false;
Led7Segment dp(GPIOC, GPIO_PIN_5, GPIOC, GPIO_PIN_7, GPIOC, GPIO_PIN_6);

I2cBus i2c1(I2C1, GPIOB, GPIO_PIN_6, GPIOB, GPIO_PIN_7);
Rtc rtc(&i2c1);

SpiBus spi1_bus(SPI1, GPIOA, GPIO_PIN_5, GPIOA, GPIO_PIN_6, GPIOA, GPIO_PIN_7); // Cho SdCard
SpiBus spi2_bus(SPI2, GPIOB, GPIO_PIN_13, GPIOB, GPIO_PIN_14, GPIOB, GPIO_PIN_15); // Cho Mp3
SpiBus spi3_bus(SPI3, GPIOB, GPIO_PIN_3, GPIOB, GPIO_PIN_4, GPIOB, GPIO_PIN_5); // Cho Rfid

Rfid rfid(&spi3_bus, GPIOA, GPIO_PIN_15, GPIOC, GPIO_PIN_14); // cs,rst
Mp3 mp3(&spi2_bus, GPIOC, GPIO_PIN_2, GPIOA, GPIO_PIN_0, GPIOA, GPIO_PIN_1, GPIOB, GPIO_PIN_12, GPIOC, GPIO_PIN_3); //pwr,xrs,xdcs,xcs
SdCard sdCard(&spi1_bus, GPIOC, GPIO_PIN_8, GPIOB, GPIO_PIN_0, GPIOB, GPIO_PIN_1); // cs,sd_dt,pwr

extern CAN_HandleTypeDef hcan;
CanBus myCan(&hcan);

volatile uint16_t productCount = 0;
volatile uint16_t cycleCount = 0;
uint16_t mucTieu = 10;
bool datMucTieu = false;
uint32_t lastCanSend = 0;
uint8_t uidTheHienTai[5];
uint8_t uidTheDaXuLy[5] = {0}; 
bool coTheRfid = false;

//can+rfid
uint8_t trangThaiQuetThe = 0; //0 binh thuong, 1: cho doi tinh nang, 2: cho xoa the, 3: cho doi key
uint8_t updateDuLieuThe=0; //1: reset, 2: bat relay, 3:dao relay, 4: phat nhac
uint8_t matKhauTheMoi[6] = {0};//luu mat khau moi 
uint8_t matKhauTam[6]={0}; //mat khau cu cua the la
bool coMatKhauTam = false;

uint32_t lastRelayDao = 0;
bool choCapNhatManHinh = false;

//kích hoạt test cảm biến
#define ENABLE_SENSOR_DIAGNOSTIC

#ifdef ENABLE_SENSOR_DIAGNOSTIC
// Biến phục vụ chức năng Diagnostic cảm biến
bool cheDoChanDoan = true; 
uint32_t thoiDiemDoiTrangThai = 0;
uint32_t thoiDiemVao = 0;
uint32_t thoiDiemRa = 0;
uint32_t thoiGianBiChe = 0;
uint32_t thoiGianTrong = 0;
uint32_t thoiGianBiCheTruoc = 0;
uint32_t thoiGianTrongTruoc = 0;
bool coDuLieuMoi = false;
#endif


FATFS fs;       
FIL fil;        
FRESULT fres;   
UINT bytesWrote;

DIR rootDir;


bool daNhanTheSD = false;
volatile bool sdCardChanged = true; // Ban đầu bằng true để tự động check lần đầu
volatile uint32_t sdChangeTick = 0;
bool sdCardInSlot = false; // Biến theo dõi trạng thái vật lý của thẻ trong khe
uint32_t showMountTimeUntil = 0; // Biến lưu thời điểm kết thúc hiển thị thời gian kết nối thẻ

extern "C" void EXTI0_IRQHandler(void) {
    if (EXTI->PR & (1 << 0)) {
        EXTI->PR = (1 << 0); // Xóa cờ ngắt
        sdCardChanged = true;
        sdChangeTick = HAL_GetTick();
    }
}
uint32_t lastSdCheck = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);

extern "C" void EXTI15_10_IRQHandler(void) {
    cb.handleInterrupt(1 << 13);
}

void onSensorDetect() {
    uint32_t currentTick = HAL_GetTick();
    bool isPinHigh = (GPIOC->IDR & (1 << 13)) != 0;
    
#ifdef ENABLE_SENSOR_DIAGNOSTIC
    if (cheDoChanDoan) {
        static bool lastDiagState = false;
        if (currentTick - thoiDiemDoiTrangThai > 20) { 
            if (isPinHigh != lastDiagState) {
                if (isPinHigh) {
                  // Cảm biến bị che (HIGH)
                  if (thoiDiemRa > 0) {
                      thoiGianTrongTruoc = thoiGianTrong;
                      thoiGianTrong = currentTick - thoiDiemRa;
                  }
                  thoiDiemVao = currentTick;
              } else {
                  // Cảm biến hết bị che (LOW)
                  if (thoiDiemVao > 0) {
                      thoiGianBiCheTruoc = thoiGianBiChe;
                      thoiGianBiChe = currentTick - thoiDiemVao;
                      coDuLieuMoi = true;
                  }
                  thoiDiemRa = currentTick;
              }
                lastDiagState = isPinHigh;
                thoiDiemDoiTrangThai = currentTick;
            }
        }
    }
#endif
    
    if (isPinHigh) {
        productCount++;
        if (productCount > 9999) {
            productCount = 0;
            cycleCount++;
            BKP->DR2 = cycleCount;
        }
        BKP->DR1 = productCount;
    }
}
static void MX_GPIO_Init(void);
static void MX_CAN_Init(void);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_CAN_Init();
  MX_FATFS_Init();
  /* USER CODE BEGIN 2 */

  //Kích hoạt nguồn và Clock cho Backup Register. Nạp lại biến ầm từ thanh ghi BKP->DR1
  RCC->APB1ENR |= (RCC_APB1ENR_PWREN | RCC_APB1ENR_BKPEN); // Cấp xung nhịp cho PWR và BKP
  PWR->CR |= PWR_CR_DBP; // Mở khóa cho phép ghi vào phần vùng Backup (Disable Backup Domain write protection)
  
  productCount = BKP->DR1;
  cycleCount = BKP->DR2;

  ledOb.init();
#ifdef ENABLE_SENSOR_DIAGNOSTIC
  cb.init(Sensor::BOTH_EDGES); 
#else
  cb.init(Sensor::RISING_EDGE); 
#endif

  cb.setCallback(onSensorDetect);
  dp.init();
  rtc.init();
  buzzer.init();
  rfid.init();
  relay.init();
  mp3.init();
  myCan.init();
  sdCard.init();
  
  // Kích hoạt DMA cho spi1
  spi1_bus.EnableDMA(true);
  
  // Cấu hình ngắt EXTI0 cho chân PB0 (SD_DETECT)
  AFIO->EXTICR[0] &= ~(0xF << 0);
  AFIO->EXTICR[0] |= (0x1 << 0); // Chọn Port B
  EXTI->IMR |= (1 << 0);
  EXTI->RTSR |= (1 << 0); // Ngắt cạnh lên (Rớt thẻ)
  EXTI->FTSR |= (1 << 0); // Ngắt cạnh xuống (Cắm thẻ)
  NVIC_EnableIRQ(EXTI0_IRQn);
  NVIC_SetPriority(EXTI0_IRQn, 5); // Ưu tiên trung bình

  //còi khởi động
  buzzer.beep(2, 50, 100, true);
    HAL_Delay(300); // Chờ tiếng bíp khởi động hơn tắt

  fres = f_mount(&fs, "", 1);
  if (fres != FR_OK) {
        buzzer.beep(3, 100, 100);
    }

  if (rtc.isTimeLost()) {
        rtcErrorFlag = true;
    }
  
  dp.setColon(false);
  dp.setValue(productCount);
  dp.update();
  
  
  uint32_t lastDisplayToggle = 0;
  bool isShowingTime = false;
  uint16_t lastDisplayValue = 0xFFFF;
  bool lastBlinkState = false;
  
  
  uint32_t lastCardRead = 0;
  uint16_t timeValue = 0;
  
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    static bool lastCanError = false;
    bool currentCanError = (CAN1->ESR & (CAN_ESR_BOFF | CAN_ESR_EPVF)) != 0;
    if (currentCanError && !lastCanError) {
        buzzer.beep(1, 500, 0); // Bao mat CAN 1 tieng bip dai
    }
    // xóa cờ lỗi khi can kết nối lại
    if (!currentCanError && lastCanError) {
        hcan.ErrorCode = HAL_CAN_ERROR_NONE;
        hcan.State = HAL_CAN_STATE_LISTENING;
    }
    lastCanError = currentCanError;

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    
#ifdef ENABLE_SENSOR_DIAGNOSTIC
    //ĐỌC LOG CẢM BIẾN (DIAGNOSTIC)
    uint16_t currBlock = 0, currGap = 0, prevBlock = 0, prevGap = 0;
    if (coDuLieuMoi) {
        currBlock = (thoiGianBiChe > 9999) ? 9999 : thoiGianBiChe;
        currGap = (thoiGianTrong > 9999) ? 9999 : thoiGianTrong;
        prevBlock = (thoiGianBiCheTruoc > 9999) ? 9999 : thoiGianBiCheTruoc;
        prevGap = (thoiGianTrongTruoc > 9999) ? 9999 : thoiGianTrongTruoc;
        coDuLieuMoi = false;
        // Gửi qua CAN ID: 0x124 
        // Byte 0-1: Current Block, Byte 2-3: Current Gap
        // Byte 4-5: Previous Block, Byte 6-7: Previous Gap
        uint8_t canData[8] = {0};
        
        canData[0] = currBlock & 0xFF;
        canData[1] = (currBlock >> 8) & 0xFF;
        
        canData[2] = currGap & 0xFF;
        canData[3] = (currGap >> 8) & 0xFF;
        
        canData[4] = prevBlock & 0xFF;
        canData[5] = (prevBlock >> 8) & 0xFF;
        
        canData[6] = prevGap & 0xFF;
        canData[7] = (prevGap >> 8) & 0xFF;
        myCan.sendFrame(0x124, canData, 8);
    }
#endif
    
    //chong nhay led
    uint32_t currentTick = HAL_GetTick();
    
    if (choCapNhatManHinh) {
        if (currentTick - lastRelayDao >= 100) {
            dp.update(isShowingTime);
            choCapNhatManHinh = false;
        }
    }
    //kiem tra the nho
    if (sdCardChanged) { 
        // Lọc nhiễu chống rung 100ms cho tiếp điểm thẻ nhớ
        if (currentTick - sdChangeTick >= 100) {
            sdCardChanged = false;
            
            bool cardPresent = sdCard.checkPhysicalPresence();
            
            if (!cardPresent) {
                // Rút thẻ
                if (daNhanTheSD || sdCardInSlot) {
                    GPIOA->BSRR = (1 << 4); // Tắt Amply (Mute) để chống tiếng bíp
                    daNhanTheSD = false;
                    sdCardInSlot = false;
                    mp3.saveState(); // Lưu trạng thái bị ngắt để phát tiếp
                    mp3.init();
                    f_mount(NULL, "", 0); 
                    buzzer.beep(1, 100, 0, true); // Kêu 1 tiếng bíp hiệu rút thẻ
                    HAL_Delay(100);
                    GPIOA->BRR = (1 << 4); // Bật lại Amply
                }
            } else {
                // Cắm thẻ
                if (!sdCardInSlot) {
                    GPIOA->BSRR = (1 << 4); // Tắt Amply (Mute) để chống nhiễu điện khi cắm nóng
                    sdCardInSlot = true; // đánh dấu là đã cắm vào
                    uint32_t startMount = HAL_GetTick();
                
                    sdCard.init();
                
                    FATFS_UnLinkDriver(USERPath);
                    FATFS_LinkDriver(&USER_Driver, USERPath);
                
                    // Chờ chút ổn định điện áp sau khi cắm nóng
                    HAL_Delay(300); // Tăng lên 300ms cho các thẻ dung lượng cao
                
                    uint8_t stat = sdCard.initialize();
                
                    if (stat == 0x00) { // Khởi tạo SD thành công
                        if (f_mount(&fs, USERPath, 1) == FR_OK) {
                            daNhanTheSD = true;
                              uint32_t mountTime = HAL_GetTick() - startMount;
                          
                              // Kêu 2 tiếng ngắn báo thành công
                          buzzer.beep(2, 50, 100, true); 
                          
                          // Hiển thị thời gian kết nối (ms) lên màn hình LED trong 3 giây
                          dp.setColon(false);
                          dp.setValue(mountTime);
                          dp.update(false);
                          lastDisplayValue = mountTime;
                          showMountTimeUntil = HAL_GetTick() + 3000;
                          
                          // Gửi thời gian kết nối (ms) qua mạng CAN (ID 0x125)
                          // Định dạng Little-Endian 32-bit Integer để Cangaroo DBC giải mã chuẩn
                          uint8_t canData[4];
                          canData[0] = mountTime & 0xFF;
                          canData[1] = (mountTime >> 8) & 0xFF;
                          canData[2] = (mountTime >> 16) & 0xFF;
                          canData[3] = (mountTime >> 24) & 0xFF;
                          myCan.sendFrame(0x125, canData, 4);
                          
                          // Khi phục hồi biết đang nghe dở
                          mp3.resumeState();
                        } else {
                            // Lỗi File System
                            buzzer.beep(3, 100, 100);
                        }
                    } else {
                        // Lỗi phần cứng thẻ (Thẻ hỏng hoặc không tương thích)
                        // Còi kêu 5 tiếng nhanh báo hiệu thẻ hỏng
                        buzzer.beep(5, 50, 50);
                    }
                    
                    HAL_Delay(100);
                    GPIOA->BRR = (1 << 4); // Bật lại Amply sau khi nạp thẻ xong
                } // Kết thúc if (!sdCardInSlot)
            }
        }
    }
    
    if (daNhanTheSD) {
        mp3.process();
    }

    //1:kiem tra cam bien
    if (cb.isDetected) {
      cb.isDetected = false;
      if ((GPIOC->IDR & (1 << 13)) != 0) { // Cảm biến tích cực mức CAO (HIGH) khi có vật cản
        buzzer.beep(1, 50, 0, true);
      // Chuyển màn hình về đếm số ngay lập tức
      isShowingTime = false;
      lastDisplayToggle = currentTick;
      lastDisplayValue = productCount;
      dp.setColon(false);
      dp.setValue(productCount);
      dp.update(false); // Cập nhật lại số đếm
      }
    }
      if(productCount >= mucTieu && !datMucTieu){
        datMucTieu = true;
        if (!mp3.play("DAT.MP3")) { 
          buzzer.beep(3, 100, 100); 
        }
      }
    //2:doi mode led
    if (currentTick - lastDisplayToggle >= 5000)
    {
      isShowingTime = !isShowingTime;   
      lastDisplayToggle = currentTick;  
      lastDisplayValue = 0xFFFF;        
    }
    //3:hien thi led
    if (currentTick < showMountTimeUntil) {
        // đang hiển thị thời gian kết nối thẻ, tạm thời giữ nguyên màn hình LED
    }
    else if (isShowingTime)
    {
      bool currentBlinkState = (currentTick / 500) % 2 == 0;
      
      if (rtcErrorFlag) {
          if (currentBlinkState != lastBlinkState) {
              if (currentBlinkState) {
                  dp.showDashes();
              } else {
                  dp.blank();
              }
              lastBlinkState = currentBlinkState;
              lastDisplayValue = 0xFFFE;
          }
      }
      else {
          static uint32_t lastRtcReadTime = 0;
          if (currentTick - lastRtcReadTime >= 1000 || lastRtcReadTime == 0) {
              uint8_t curH = 0, curM = 0, curS = 0;
              rtc.getTime(curH, curM, curS);
              timeValue = (curH * 100) + curM;
              lastRtcReadTime = currentTick;
          }
          
          if ((timeValue != lastDisplayValue) || (currentBlinkState != lastBlinkState)) {
              dp.setColon(currentBlinkState);     
              dp.setValue(timeValue);    
              dp.update(true);    
              lastDisplayValue = timeValue;
              lastBlinkState = currentBlinkState;
          }
      }
    }
    else
    {
      bool currentBlinkState = false;
      if (CAN1->ESR & (CAN_ESR_BOFF | CAN_ESR_EPVF)) {
          currentBlinkState = (currentTick / 100) % 2 == 0; //chớp dấu : mode đếm cảnh báo mất canbus
      }
      
      if (productCount != lastDisplayValue || currentBlinkState != lastBlinkState) 
      {
          dp.setColon(currentBlinkState);   
          dp.setValue(productCount);    
          dp.update(false);
          lastDisplayValue = productCount;
          lastBlinkState = currentBlinkState;
      }
    }
    // 4: doc du lieu the
    if (currentTick - lastCardRead >= 100) {
        lastCardRead = currentTick; 
        if (rfid.checkCard()) {
            if (rfid.readCardSerial((uint8_t*)uidTheHienTai)) {
                
                if (!coTheRfid || memcmp(uidTheHienTai, uidTheDaXuLy, 5) != 0) {
                    
                    // update the hien tai vao bo nho
                    memcpy(uidTheDaXuLy, uidTheHienTai, 5);
                    coTheRfid = true;
                    buzzer.beep(1, 100, 0, true);
                    
                    uint8_t currentKeyA[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
                    uint8_t readStatus = 0xFF; 
                    uint8_t readData[18] = {0};

                    if (rfid.selectTag((uint8_t*)uidTheHienTai) > 0) {                    
                        uint8_t blockAddr = 4;
                        uint8_t trailerBlock = 7;
                        uint8_t status;
                        
                        // chon key
                        if (coMatKhauTam) {
                            status = rfid.PCD_Authenticate(0x60, blockAddr, matKhauTam, (uint8_t*)uidTheHienTai);
                        } else {
                            status = rfid.PCD_Authenticate(0x60, blockAddr, currentKeyA, (uint8_t*)uidTheHienTai);
                        }
                        
                        // update the
                        if (status == STATUS_OK) {
                            if (trangThaiQuetThe > 0) {
                                if (trangThaiQuetThe == 1 || trangThaiQuetThe == 2) {
                                    // xoa the hoac ghi tinh nang
                                    uint8_t writeData[16] = {0};
                                    if (trangThaiQuetThe == 1) {
                                        if (updateDuLieuThe == 1) strncpy((char*)writeData, "RESET_DEM", 9);
                                        else if (updateDuLieuThe == 2) strncpy((char*)writeData, "BAT_RELAY", 9);
                                        else if (updateDuLieuThe == 3) strncpy((char*)writeData, "DAO_RELAY", 9);
                                        else if (updateDuLieuThe == 4) strncpy((char*)writeData, "PHAT_NHAC", 9);
                                    }
                                    status = rfid.MIFARE_Write(blockAddr, writeData, 16);
                                    if (status == STATUS_OK) (void)0; // buzzer.beep(1, 500, 0); 
                                    else (void)0; // buzzer.beep(2, 100, 100); 
                                }
                                else if (trangThaiQuetThe == 3) {
                                    uint8_t trailerData[16] = {
                                        matKhauTheMoi[0], matKhauTheMoi[1], matKhauTheMoi[2], matKhauTheMoi[3], matKhauTheMoi[4], matKhauTheMoi[5], 
                                        0xFF, 0x07, 0x80, 0x69,
                                        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF  // Key B
                                    };
                                    status = rfid.MIFARE_Write(trailerBlock, trailerData, 16);
                                    if (status == STATUS_OK) (void)0; // buzzer.beep(1, 500, 0);
                                    else (void)0; // buzzer.beep(2, 100, 100);
                                }
                                // don dep co
                                trangThaiQuetThe = 0; 
                                coMatKhauTam = false;
                                readStatus = 0x00;
                            } 
                            else {
                                uint8_t size = sizeof(readData);
                                status = rfid.MIFARE_Read(blockAddr, readData, &size);
                                
                                if (status == STATUS_OK) {
                                    if (strncmp((char*)readData, "RESET_DEM", 9) == 0) {
                                        if (productCount == 0) {
                                            buzzer.beep(2, 100, 100); // đã reset rồi, bỏ qua
                                            readStatus = 0x01;
                                        } else {
                                            if (!mp3.play("RESET.MP3")) { 

                                              buzzer.beep(3, 100, 100); 
                                            }
                                            //Cập nhật lại thanh ghi BKP khi Reset đểm
                                            productCount = 0;
                                            cycleCount = 0;
                                            BKP->DR1 = productCount;
                                            BKP->DR2 = cycleCount;
                                            lastDisplayValue = 0xFFFF;
                                            readStatus = 0x00;
                                            datMucTieu = false;
                                        }
                                    }
                                    else if (strncmp((char*)readData, "BAT_RELAY", 9) == 0) {
                                        if (relay.isOn()) {
                                            buzzer.beep(2, 100, 100); // đã bật rồi, bỏ qua
                                            readStatus = 0x01;
                                        } else {
                                            char uidFilename[13];
                                            snprintf(uidFilename, sizeof(uidFilename), "%02X%02X%02X%02X.MP3", 
                                                     uidTheHienTai[0], uidTheHienTai[1], uidTheHienTai[2], uidTheHienTai[3]);
                                                     
                                            if (!mp3.play(uidFilename)) { 
                                                if (!mp3.play("BAT.MP3")) {
                                                    buzzer.beep(3, 100, 100); 
                                                }
                                            }
                                            dp.turnOff();
                                            relay.on();
                                            lastRelayDao = currentTick;
                                            choCapNhatManHinh = true;
                                            readStatus = 0x00;
                                        }
                                    }
                                    else if (strncmp((char*)readData, "DAO_RELAY", 9) == 0) {
                                        char uidFilename[13];
                                        snprintf(uidFilename, sizeof(uidFilename), "%02X%02X%02X%02X.MP3", 
                                                 uidTheHienTai[0], uidTheHienTai[1], uidTheHienTai[2], uidTheHienTai[3]);
                                                 
                                        if (!mp3.play(uidFilename)) { 
                                            if (!mp3.play("DAO.MP3")) {
                                                buzzer.beep(3, 100, 100); 
                                            }
                                        }
                                        dp.turnOff();
                                        relay.toggle();
                                        lastRelayDao = currentTick;
                                        choCapNhatManHinh = true;
                                        readStatus = 0x00;
                                    }
                                    else if (strncmp((char*)readData, "PHAT_NHAC", 9) == 0) {
                                        if (!mp3.play("MUSIC.MP3")) { 

                                          buzzer.beep(3, 100, 100); 
                                        }
                                        readStatus = 0x00;
                                    }
                                    else {
                                        buzzer.beep(2, 100, 100);
                                        readStatus = 0x01; // sai du lieu
                                    }
                                } else {
                                    buzzer.beep(2, 100, 100);
                                    readStatus = 0x02; // ko doc duoc block
                                }
                            }
                        } else {
                            buzzer.beep(2, 100, 100);
                            readStatus = 0x03; // sai mat khau
                            trangThaiQuetThe = 0; 
                            coMatKhauTam = false;
                        }
                    }
                    
                    // Frame 0x101: uid + trang thai doc
                    uint8_t frame101[6];
                    memcpy(frame101, uidTheHienTai, 5);
                    frame101[5] = readStatus; 
                    myCan.sendFrame(0x101, frame101, 6);
                    
                    if (readStatus == 0x00 || readStatus == 0x01) {
                        myCan.sendFrame(0x104, &readData[0], 8);
                        myCan.sendFrame(0x105, &readData[8], 8);
                    }
                    
                    rfid.PICC_HaltA();
                      rfid.PCD_StopCrypto1();
                  } else {
                      if (rfid.selectTag((uint8_t*)uidTheHienTai) > 0) {
                          rfid.PICC_HaltA();
                          rfid.PCD_StopCrypto1();
                      }
                  }
              }
          } else {
              coTheRfid = false;
          }
    }
    
    // Doc RTC moi 500ms de tranh nghe I2C (Blocking) lam cham he thong
    static uint32_t lastRtcRead = 0;
    static uint8_t h = 0, m = 0, s = 0, dow = 0, date = 0, month = 0, year = 0;
    if (currentTick - lastRtcRead >= 500) {
        rtc.getTime(h, m, s);
        rtc.getDate(dow, date, month, year);
        lastRtcRead = currentTick;
    }

    //6:gui du lieu can rtc vs cam bien dinh ky 100ms
    static uint32_t lastCanSend = 0;
    if (currentTick - lastCanSend >= 100) {
        lastCanSend = currentTick;
        uint8_t frame102[6] = {0};
        uint16_t currentCount = productCount; 
        uint16_t currentTarget = mucTieu;
        // Đổi sang dạng Little Endian để trùng khớp với Cangaroo (.dbc)
        frame102[0] = currentCount & 0xFF;
        frame102[1] = (currentCount >> 8) & 0xFF;
        frame102[2] = currentTarget & 0xFF;
        frame102[3] = (currentTarget >> 8) & 0xFF;
        frame102[4] = cycleCount & 0xFF;
        frame102[5] = (cycleCount >> 8) & 0xFF;
        
        myCan.sendFrame(0x102, frame102, 6);
        
        uint8_t frame103[7];
        // Truyền thẳng dạng thập phân (Decimal) thay vì BCD để Cangaroo hiển thị chuẩn
        frame103[0] = h; 
        frame103[1] = m; 
        frame103[2] = s; 
        frame103[3] = date;
        frame103[4] = month;
        frame103[5] = year;
        frame103[6] = dow;
        
        myCan.sendFrame(0x103, frame103, 7);
    }
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief CAN Initialization Function
  * @param None
  * @retval None
  */
static void MX_CAN_Init(void)
{
  /* USER CODE BEGIN CAN_Init 0 */

  /* USER CODE END CAN_Init 0 */

  /* USER CODE BEGIN CAN_Init 1 */

  /* USER CODE END CAN_Init 1 */
  hcan.Instance = CAN1;
  hcan.Init.Prescaler = 9;
  hcan.Init.Mode = CAN_MODE_NORMAL;
  hcan.Init.SyncJumpWidth = CAN_SJW_1TQ;
  hcan.Init.TimeSeg1 = CAN_BS1_6TQ; // 500kbps (36MHz / 9 / (1 + 6 + 1))
  hcan.Init.TimeSeg2 = CAN_BS2_1TQ;
  hcan.Init.TimeTriggeredMode = DISABLE;
  hcan.Init.AutoBusOff = ENABLE;
  hcan.Init.AutoWakeUp = DISABLE;
  hcan.Init.AutoRetransmission = ENABLE;
  hcan.Init.ReceiveFifoLocked = DISABLE;
  hcan.Init.TransmitFifoPriority = DISABLE;
  if (HAL_CAN_Init(&hcan) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CAN_Init 2 */

  /* USER CODE END CAN_Init 2 */
}


// Đã xóa toàn bộ hàm MX_SPI2_Init vì thư viện SpiBus đã tự cấu hình Bare-metal

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /* USER CODE BEGIN MX_GPIO_Init_2 */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE(); 
  
  //Cau hinh PA4 (Output Push-Pull, 2MHz)
  GPIOA->CRL = (GPIOA->CRL & ~(0xF << 16)) | (0x2 << 16);
  GPIOA->BSRR = (1 << 4); // Khởi động mặc định ở trạng thái Tắt Amply (Mute) để triệt tiêu tiếng bíp
  
  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
extern "C" {
    void HAL_IncTick(void) {
        uwTick += uwTickFreq;
        
        buzzer.update();
        
        static uint32_t lastLedNhay = 0;
        if (uwTick - lastLedNhay >= 100) {
            ledOb.toggle();
            lastLedNhay = uwTick;
        }
    }
    
    bool isValidBCD(uint8_t bcd) {
        return ((bcd >> 4) <= 9) && ((bcd & 0x0F) <= 9);
    }

    // 2. bcd-decimal
    uint8_t BCD_to_Dec(uint8_t bcd) {
        return ((bcd >> 4) * 10) + (bcd & 0x0F);
    }

    void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
    {
        uint32_t rxId;
        uint8_t rxData[8];
        uint8_t rxLen = 0;

        if (myCan.readFrame(&rxId, rxData, &rxLen)) {
          //0x201 target
            if (rxId == 0x201) {
                uint16_t val = rxData[0] | (rxData[1] << 8);
                if (val <= 9999) {
                        mucTieu = val;
                        datMucTieu = false;
                        buzzer.beep(1, 150, 0);
                    }else buzzer.beep(2, 100, 100);
            }
            //0x202 dem hien tai
            else if (rxId == 0x202) {
                uint16_t val = rxData[0] | (rxData[1] << 8);
                if (val <= 9999) {
                    // Cập nhật lại thanh ghi BKP khi set giá trị đếm từ mạng CAN
                    productCount = val;
                    BKP->DR1 = productCount;
                    datMucTieu = false;
                    buzzer.beep(1, 150, 0);
                } else {
                    buzzer.beep(2, 100, 100);
                }
            }
            //0x203 time
            else if (rxId == 0x203) {
                if (rxLen >= 3) {
                    uint8_t h = rxData[0];
                    uint8_t m = rxData[1];
                    uint8_t s = rxData[2];
                    if (h <= 23 && m <= 59 && s <= 59) {
                        rtc.setTime(h, m, s);
                        rtcErrorFlag = false;
                        if (rxLen >= 7) {
                            // Đọc đúng thứ tự của .dbc: Ngày(3), Tháng(4), Năm(5), Thứ(6)
                            uint8_t date = rxData[3];
                            uint8_t month = rxData[4];
                            uint8_t year = rxData[5];
                            uint8_t dow = rxData[6];
                            rtc.setDate(dow, date, month, year);
                        }
                        buzzer.beep(1, 150, 0);
                    } else buzzer.beep(2, 100, 100);
                } else buzzer.beep(2, 100, 100);
            }
              //thay doi tinh nang
            else if (rxId == 0x204){
              if(rxData[0] >=1 && rxData[0]<=4){
                trangThaiQuetThe = 1;
                updateDuLieuThe = rxData[0];
                buzzer.beep(2,50,50);
              }else buzzer.beep(3,100,100);  
            }
            //0x205 xoa the,doi key
            else if(rxId == 0x205){
              if(rxData[0] == 0x00){ //xoa the
                trangThaiQuetThe = 2;
                buzzer.beep(2,50,50);
              }
              else if (rxData[0] == 0x01) {//doi key
                trangThaiQuetThe = 3;
                memcpy(matKhauTheMoi, &rxData[1],6);
                buzzer.beep(2,50,50);
              }
              else buzzer.beep(3,100,100); 
            }
            else if (rxId == 0x206){
              memcpy(matKhauTam, rxData,6);
              coMatKhauTam = true;
              buzzer.beep(2,50,50);
            }
        }
    }
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
























