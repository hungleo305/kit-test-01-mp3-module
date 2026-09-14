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

#include "CanBus.h"
#include "SpiBus.h"
#include "I2cBus.h"
#include "SdCard.h"

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
Sensor cb2(GPIOB, GPIO_PIN_8);
Relay relay(GPIOC, GPIO_PIN_7);

volatile bool rtcErrorFlag = false;

Led7Segment dp(GPIOC, GPIO_PIN_5, GPIOC, GPIO_PIN_7, GPIOC, GPIO_PIN_6, 9);
//Led7Segment dp2(GPIOA, GPIO_PIN_8, GPIOC,GPIO_PIN_10,GPIOC,GPIO_PIN_11,4);
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
volatile uint16_t productCount2 = 0;
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

bool choCapNhatManHinh = false;




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
extern "C" void EXTI9_5_IRQHandler(void) {
    cb2.handleInterrupt(1 << 8);
    
}

static void MX_GPIO_Init(void);
static void MX_CAN_Init(void);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
// void testSdCardSpeedAndSendCan() {
//     static uint8_t buffer[4096]; // Dung static de tranh tran Stack, 4KB buffer (8 block) de ep DMA chay max cong suat
//     for (int i = 0; i < 4096; i++) buffer[i] = 0xAA;
    
//     uint32_t totalBlocks = 128; // 128 blocks = 64KB
//     uint32_t loopCount = totalBlocks / 8; // 16 loops (mỗi loop 8 block = 4KB)
    
//     uint32_t start, timeMs, speedKBs;
//     uint8_t frame[7] = {0};
//     uint32_t timeout;
    
//     FIL testFile;
//     UINT bw, br;
    
//     // 1. TEST GHI FATFS (Tốc độ thực tế) -> 0x127
//     if (f_open(&testFile, "speed.tst", FA_WRITE | FA_CREATE_ALWAYS) == FR_OK) {
//         start = HAL_GetTick();
//         for (uint32_t i = 0; i < loopCount; i++) {
//             f_write(&testFile, buffer, 4096, &bw);
//         }
//         f_close(&testFile);
//         timeMs = HAL_GetTick() - start;
//         if (timeMs == 0) timeMs = 1;
//         speedKBs = (totalBlocks * 512 * 1000) / 1024 / timeMs;
        
//         frame[0] = 1; // Trang_Thai = 1 (Thực tế)
//         frame[1] = timeMs & 0xFF; frame[2] = (timeMs >> 8) & 0xFF;
//         frame[3] = totalBlocks & 0xFF; frame[4] = (totalBlocks >> 8) & 0xFF;
//         frame[5] = speedKBs & 0xFF; frame[6] = (speedKBs >> 8) & 0xFF;
//     } else {
//         frame[0] = 0; // Error
//     }
//     timeout = HAL_GetTick();
//     while(!myCan.sendFrame(0x127, frame, 7)) { if (HAL_GetTick() - timeout > 5) break; }

//     // 2. TEST ĐỌC FATFS (Tốc độ thực tế) -> 0x126
//     if (f_open(&testFile, "speed.tst", FA_READ) == FR_OK) {
//         start = HAL_GetTick();
//         for (uint32_t i = 0; i < loopCount; i++) {
//             f_read(&testFile, buffer, 4096, &br);
//         }
//         f_close(&testFile);
//         timeMs = HAL_GetTick() - start;
//         if (timeMs == 0) timeMs = 1;
//         speedKBs = (totalBlocks * 512 * 1000) / 1024 / timeMs;
        
//         frame[0] = 1; // Trang_Thai = 1 (Thực tế)
//         frame[1] = timeMs & 0xFF; frame[2] = (timeMs >> 8) & 0xFF;
//         frame[3] = totalBlocks & 0xFF; frame[4] = (totalBlocks >> 8) & 0xFF;
//         frame[5] = speedKBs & 0xFF; frame[6] = (speedKBs >> 8) & 0xFF;
//     } else {
//         frame[0] = 0; // Error
//     }
//     timeout = HAL_GetTick();
//     while(!myCan.sendFrame(0x126, frame, 7)) { if (HAL_GetTick() - timeout > 5) break; }
    
//     f_unlink("speed.tst");

//     // 3. TEST GHI RAW (Tốc độ phần cứng MAX) -> 0x129

//     start = HAL_GetTick();
//     uint8_t stat = 0;
    
//     // Chia nhỏ 128 blocks thành các vòng lặp, mỗi vòng lặp ghi 8 blocks (4096 bytes) để vừa với buffer
//     for (uint32_t i = 0; i < loopCount; i++) {
//         // Tịnh tiến địa chỉ sector lên 8 sau mỗi lần ghi: 200000, 200008, 200016...
//         if (sdCard.write(buffer, 200000 + (i * 8), 8) != 0) { 
//             stat = 1; // Lỗi
//             break; 
//         }
//     }
    
//     timeMs = HAL_GetTick() - start;
//     if (timeMs == 0) timeMs = 1;
//     speedKBs = (totalBlocks * 512 * 1000) / 1024 / timeMs;
    
//     if (stat == 0) {
//         frame[0] = 2; // Trang_Thai = 2 (Raw)
//         frame[1] = timeMs & 0xFF; frame[2] = (timeMs >> 8) & 0xFF;
//         frame[3] = totalBlocks & 0xFF; frame[4] = (totalBlocks >> 8) & 0xFF;
//         frame[5] = speedKBs & 0xFF; frame[6] = (speedKBs >> 8) & 0xFF;
//     } else {
//         frame[0] = 0; // Error
//     }
//     timeout = HAL_GetTick();
//     while(!myCan.sendFrame(0x129, frame, 7)) { if (HAL_GetTick() - timeout > 5) break; }

//     // 4. TEST ĐỌC RAW (Tốc độ phần cứng MAX) -> 0x12A
// start = HAL_GetTick();
//     stat = 0;
    
//     // Tương tự, đọc từng cụm 8 blocks vào buffer
//     for (uint32_t i = 0; i < loopCount; i++) {
//         if (sdCard.read(buffer, 200000 + (i * 8), 8) != 0) { 
//             stat = 1; // Lỗi
//             break; 
//         }
//     }
    
//     timeMs = HAL_GetTick() - start;
//     if (timeMs == 0) timeMs = 1;
//     speedKBs = (totalBlocks * 512 * 1000) / 1024 / timeMs;
    
//     if (stat == 0) {
//         frame[0] = 2; // Trang_Thai = 2 (Raw)
//         frame[1] = timeMs & 0xFF; frame[2] = (timeMs >> 8) & 0xFF;
//         frame[3] = totalBlocks & 0xFF; frame[4] = (totalBlocks >> 8) & 0xFF;
//         frame[5] = speedKBs & 0xFF; frame[6] = (speedKBs >> 8) & 0xFF;
//     } else {
//         frame[0] = 0; // Error
//     }
//     timeout = HAL_GetTick();
//     while(!myCan.sendFrame(0x12A, frame, 7)) { if (HAL_GetTick() - timeout > 5) break; }
// }
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
    productCount2 = BKP->DR3;

  ledOb.init();
  cb.init(Sensor::RISING_EDGE);
  cb2.init(Sensor::RISING_EDGE); 

  dp.setSegmentMap(5, 4, 0, 1, 2, 6, 7, 3);
  dp.setReverseMask(1 << 1); // Đảo chiều digit ở vị trí index 1 (hàng chục của cụm 4 led)
  dp.setInverted(false);

  dp.init(); 
  
  rtc.init();
  buzzer.init();
  rfid.init();
  relay.init();
  mp3.init();
  mp3.setPlaySpeed(2.0f);
  mp3.setVolume(40,40);
  myCan.init();
  sdCard.init();
  
  // Kích hoạt DMA cho spi1
  spi1_bus.EnableDMA(true);
  spi2_bus.EnableDMA(true); // Fix MP3 stuttering
  
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
    } else {
        daNhanTheSD = true;
       // testSdCardSpeedAndSendCan(); //test the sd
    }

  if (rtc.isTimeLost()) {
        rtcErrorFlag = true;
    } 
  
  uint32_t lastDisplayToggle = 0;
  bool isShowingTime = false;
  uint32_t lastUiTick = 0;
  

  
  uint32_t lastCardRead = 0;
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    
    //Báo lỗi kết nối canbus
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
    uint32_t currentTick = HAL_GetTick();
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
                
                    uint8_t stat = sdCard.initialize();
                
                    if (stat == 0x00) { // Khởi tạo SD thành công
                        if (f_mount(&fs, USERPath, 1) == FR_OK) {
                            daNhanTheSD = true;
                            //testSdCardSpeedAndSendCan(); //test the sd
                              uint32_t mountTime = HAL_GetTick() - startMount;
                          
                              // Kêu 2 tiếng ngắn báo thành công
                          buzzer.beep(2, 50, 100, true); 
                          
                          // Hiển thị thời gian kết nối (ms) lên màn hình LED trong 3 giây
                          dp.clearBuffer();
                          dp.setNumber(mountTime, 0, 4, false);
                          dp.refresh();
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
                    
                    GPIOA->BRR = (1 << 4); // Bật lại Amply sau khi nạp thẻ xong
                } // Kết thúc if (!sdCardInSlot)
            }
        }
    }
    
    if (daNhanTheSD) {
        mp3.process();
    }

    // 1: Kiểm tra cảm biến
    if (cb2.isDetected){
        cb2.isDetected = false;
        
        productCount2++;
        if (productCount2 > 9999) {
            productCount2 = 0;
        }
        BKP->DR3 = productCount2;
        
        buzzer.beep(1, 50, 50, true);
        lastUiTick = 0; // Ép vẽ lại màn hình ngay lập tức
    }
    if (cb.isDetected) {
      cb.isDetected = false;
      
      productCount++;
      if (productCount > 9999) {
          productCount = 0;
          cycleCount++;
          BKP->DR2 = cycleCount;
      }
      BKP->DR1 = productCount;
      
      buzzer.beep(1, 50, 50, true); 
      isShowingTime = false;
      lastDisplayToggle = currentTick;
      lastUiTick = 0; // Ép vẽ lại màn hình ngay lập tức
    }
      if(productCount >= mucTieu && !datMucTieu){
        datMucTieu = true;
        if (!mp3.play("DAT.MP3")) { 
          buzzer.beep(3, 100, 100); 
        }
      }
    // 2. CẬP NHẬT GIAO DIỆN MÀN HÌNH (10 lần/giây)
    static uint8_t h = 0, m = 0, s = 0, dow = 0, date = 0, month = 0, year = 0;
    if (currentTick - lastUiTick >= 100) {
        lastUiTick = currentTick;

        // A. Đọc thời gian RTC
        if (!rtcErrorFlag) {
            rtc.getTime(h, m, s);
            rtc.getDate(dow, date, month, year);
        }

        // C. Cập nhật Màn hình chính (dp)
        if (currentTick >= showMountTimeUntil) {
            // Chuyển đổi mode luân phiên sau mỗi 5s
            if (currentTick - lastDisplayToggle >= 5000) {
                isShowingTime = !isShowingTime;   
                lastDisplayToggle = currentTick;  
            }

            dp.clearBuffer();
            
            if (isShowingTime) {
                if (rtcErrorFlag) {
                    for (int i = 0; i < 4; i++) dp.setDigit(i, 0xFE, false);
                } else {
                    dp.setNumber((h * 100) + m, 0, 4, true); 
                    if ((currentTick / 500) % 2 == 0) {
                        dp.setDot(1, true);
                        dp.setDot(2, true);
                    }
                }
            } else {
                dp.setNumber(productCount, 0, 4, false);
                if (CAN1->ESR & (CAN_ESR_BOFF | CAN_ESR_EPVF)) {
                        dp.setDot(1, true);
                    
                }
            }
            
            if (!rtcErrorFlag) {
            dp.setNumber(s, 7, 2, true);
            }
            dp.refresh();
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
                                            productCount2 = 0;
                                            cycleCount = 0;
                                            BKP->DR1 = productCount;
                                            BKP->DR2 = cycleCount;
                                            BKP->DR3 = productCount2;
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

                                        
                                            relay.on();

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
                                       
                                       
                                        relay.toggle();
                                        
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
    

    //6:gui du lieu can rtc vs cam bien (Stress test: Gui lien tuc khong gioi han)
    {
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
    
    //7: gui thong ke CAN moi 1 giay
    static uint32_t lastCanStatsSend = 0;
    if (currentTick - lastCanStatsSend >= 1000) {
        uint8_t frame128[8] = {0};
        
        // 4 byte: So goi thanh cong
        frame128[0] = myCan.txSuccessCount & 0xFF;
        frame128[1] = (myCan.txSuccessCount >> 8) & 0xFF;
        frame128[2] = (myCan.txSuccessCount >> 16) & 0xFF;
        frame128[3] = (myCan.txSuccessCount >> 24) & 0xFF;
        
        // 4 byte: Thoi gian hoat dong (giay)
        uint32_t uptimeSec = currentTick / 1000;
        frame128[4] = uptimeSec & 0xFF;
        frame128[5] = (uptimeSec >> 8) & 0xFF;
        frame128[6] = (uptimeSec >> 16) & 0xFF;
        frame128[7] = (uptimeSec >> 24) & 0xFF;
        
        // Luôn cập nhật thời gian để không bị kẹt spam khi CAN bị ngắt
        lastCanStatsSend = currentTick;
        myCan.sendFrame(0x128, frame128, 8);
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
























