/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "SystemTimer.h"
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

SPI_HandleTypeDef hspi1;
SPI_HandleTypeDef hspi2;
/* USER CODE BEGIN PV */
Led ledOb(GPIOB, 1<<2);
Buzzer buzzer(GPIOC, 1<<15);
Sensor cb(GPIOC, 1<<13);
Relay relay(GPIOC, 1<<7);
bool getRelayState() { return relay.isOn(); }
Led7Segment dp(GPIOC, 1<<5, GPIOC, 1<<7, GPIOC, 1<<6, getRelayState);
Rtc rtc(I2C1, GPIOB, 1<<6, GPIOB, 1<<7);
Rfid rfid(SPI1, GPIOA, 1<<15, GPIOC, 1<<14);
Mp3 mp3(&hspi2, GPIOA, 1<<0, GPIOA, 1<<1, GPIOB, 1<<12, GPIOC, 1<<3);
SdCard sdCard(SPI1, GPIOC, 1<<8);

extern CAN_HandleTypeDef hcan;
CanBus myCan(&hcan);

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
        sdChangeTick = SystemTimer::getTick();
    }
}
uint32_t lastSdCheck = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_CAN_Init(void);
static void MX_SPI1_Init(void);
static void MX_SPI2_Init(void);
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
  MX_SPI1_Init();
  MX_SPI2_Init();
  MX_FATFS_Init();
  MX_CAN_Init();
  /* USER CODE BEGIN 2 */
  //Kích hoạt nguồn và Clock cho Backup Register. Nạp lại biến đếm từ thanh ghi BKP->DR1
  RCC->APB1ENR |= (RCC_APB1ENR_PWREN | RCC_APB1ENR_BKPEN); // Cấp xung nhịp cho PWR và BKP
  PWR->CR |= PWR_CR_DBP; // Mở khóa cho phép ghi vào phân vùng Backup (Disable Backup Domain write protection)
  
  Sensor::productCount = BKP->DR1;

  ledOb.init();
  cb.init();
  dp.init();
  rtc.init();
  buzzer.init();
  rfid.init();
  relay.init();
  mp3.init();
  myCan.init();
  sdCard.init();
  
  // Cấu hình ngắt EXTI0 cho chân PB0 (SD_DETECT)
  AFIO->EXTICR[0] &= ~(0xF << 0);
  AFIO->EXTICR[0] |= (0x1 << 0); // Chọn Port B
  EXTI->IMR |= (1 << 0);
  EXTI->RTSR |= (1 << 0); // Ngắt cạnh lên (Rút thẻ)
  EXTI->FTSR |= (1 << 0); // Ngắt cạnh xuống (Cắm thẻ)
  NVIC_EnableIRQ(EXTI0_IRQn);
  NVIC_SetPriority(EXTI0_IRQn, 5); // Độ ưu tiên trung bình
  buzzer.beep(2, 50, 100);
  
  /* ====== BẬT/TẮT BÀI KIỂM TRA CẢM BIẾN (DIAGNOSTIC TEST) ====== */
  /* Bỏ comment dòng dưới để BẬT chế độ đo đạc quang học cảm biến qua CAN */
  Sensor::enableDiagnosticMode(true);
  
  fres = f_mount(&fs, "", 1);
  if (fres != FR_OK) {
      buzzer.beep(3, 100, 100);
  }

  if (rtc.isTimeLost()) {
  
      buzzer.beep(3, 100, 100); 
      
      rtc.setTime(11, 7, 20); 
  }
  
  dp.setColon(false);
  dp.setValue(0);
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
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    
    /* ====== ĐỌC LOG CẢM BIẾN (DIAGNOSTIC) ====== */
    // Chỉ hoạt động khi enableDiagnosticMode(true) được gọi
    uint32_t blockTime, gapTime;
    if (Sensor::getDiagnosticData(blockTime, gapTime)) {
        // Gửi qua CAN ID: 0x124 (Định dạng BCD để đọc trực tiếp số thập phân trên Cangaroo)
        // 4 Byte đầu: Block Time (thời gian che), 4 Byte sau: Gap Time (khoảng hở)
        uint8_t canData[8] = {0};
        
        uint32_t tempBlock = blockTime;
        if (tempBlock > 99999999) tempBlock = 99999999;
        canData[3] = ((tempBlock % 100) / 10 << 4) | (tempBlock % 10); tempBlock /= 100;
        canData[2] = ((tempBlock % 100) / 10 << 4) | (tempBlock % 10); tempBlock /= 100;
        canData[1] = ((tempBlock % 100) / 10 << 4) | (tempBlock % 10); tempBlock /= 100;
        canData[0] = ((tempBlock % 100) / 10 << 4) | (tempBlock % 10);
        
        uint32_t tempGap = gapTime;
        if (tempGap > 99999999) tempGap = 99999999;
        canData[7] = ((tempGap % 100) / 10 << 4) | (tempGap % 10); tempGap /= 100;
        canData[6] = ((tempGap % 100) / 10 << 4) | (tempGap % 10); tempGap /= 100;
        canData[5] = ((tempGap % 100) / 10 << 4) | (tempGap % 10); tempGap /= 100;
        canData[4] = ((tempGap % 100) / 10 << 4) | (tempGap % 10);
        
        myCan.sendFrame(0x124, canData, 8);
    }
    
    //chong nhay led
    uint32_t currentTick = SystemTimer::getTick();
    
    if (choCapNhatManHinh) {
        if (currentTick - lastRelayDao >= 100) {
            dp.update(isShowingTime);
            choCapNhatManHinh = false;
        }
    }
    //kiem tra the nho bằng cờ ngắt (Zero-delay)
    if (sdCardChanged) { 
        // Lọc nhiễu chống rung (Debounce) 100ms cho tiếp điểm thẻ nhớ
        if (currentTick - sdChangeTick >= 100) {
            sdCardChanged = false;
            
            bool cardPresent = sdCard.checkPhysicalPresence();
            
            if (!cardPresent) {
                // Rút thẻ
                if (daNhanTheSD || sdCardInSlot) {
                    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET); // Tắt Amply (Mute) để chống tiếng bụp
                    daNhanTheSD = false;
                    sdCardInSlot = false;
                    mp3.saveState(); // Lưu trạng thái bài hát đang phát dở
                    mp3.init();
                    f_mount(NULL, "", 0); 
                    buzzer.beep(1, 100, 0); // Kêu 1 tiếng báo hiệu đã rút thẻ
                    SystemTimer::delay_ms(100);
                    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET); // Bật lại Amply
                }
            } else {
                // Cắm thẻ
                if (!sdCardInSlot) {
                    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET); // Tắt Amply (Mute) để chống nhiễu điện khi cắm nóng
                    sdCardInSlot = true; // Đánh dấu là đã có thẻ cắm vào (tránh lặp vô tận nếu thẻ hỏng)
                    uint32_t startMount = SystemTimer::getTick();
                
                    sdCard.init();
                
                    // Ép FatFs quên trạng thái phần cứng cũ
                    FATFS_UnLinkDriver(USERPath);
                    FATFS_LinkDriver(&USER_Driver, USERPath);
                
                    // Chờ một chút để điện áp ổn định sau khi cắm nóng
                    SystemTimer::delay_ms(300); // Tăng lên 300ms cho các thẻ dung lượng cao
                
                    uint8_t stat = sdCard.initialize();
                
                    if (stat == 0x00) { // Khởi tạo SD thành công
                        if (f_mount(&fs, USERPath, 1) == FR_OK) {
                            daNhanTheSD = true;
                              uint32_t mountTime = SystemTimer::getTick() - startMount;
                          
                              // Kêu 2 tiếng ngắn báo thành công
                          buzzer.beep(2, 50, 100); 
                          
                          // Hiển thị thời gian kết nối (ms) lên màn hình LED trong 3 giây
                          dp.setColon(false);
                          dp.setValue(mountTime);
                          dp.update(false);
                          lastDisplayValue = mountTime;
                          showMountTimeUntil = SystemTimer::getTick() + 3000;
                          
                          // Chuyển đổi thời gian (ms) sang định dạng BCD để hiển thị "giả" thập phân trên CAN Monitor
                          uint32_t bcdTime = 0;
                          uint32_t tempVal = mountTime;
                          int shift = 0;
                          while (tempVal > 0 && shift < 32) {
                              bcdTime |= (tempVal % 10) << shift;
                              tempVal /= 10;
                              shift += 4;
                          }
                          
                          // Gửi 4 byte qua mạng CAN (ID 0x125)
                          uint8_t canData[4];
                          canData[0] = (bcdTime >> 24) & 0xFF;
                          canData[1] = (bcdTime >> 16) & 0xFF;
                          canData[2] = (bcdTime >> 8) & 0xFF;
                          canData[3] = bcdTime & 0xFF;
                          myCan.sendFrame(0x125, canData, 4);
                          
                          // Khôi phục bài hát đang nghe dở
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
                    
                    SystemTimer::delay_ms(100);
                    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET); // Bật lại Amply sau khi nạp thẻ xong
                } // Kết thúc if (!sdCardInSlot)
            }
        }
    }
    
    if (daNhanTheSD) {
        mp3.process();
    }

    //1:kiem tra cam bien
    if (Sensor::isDetected)
    {
      buzzer.beep(1, 50, 0);
      Sensor ::isDetected =false;
    }
      if(Sensor::productCount >= mucTieu && !datMucTieu){
        datMucTieu = true;
        if (!mp3.play("DAT.MP3")) { 
          uint32_t wt = SystemTimer::getTick(); 
          while(SystemTimer::getTick() - wt < 500) buzzer.update(); 
          buzzer.beep(5, 100, 100); 
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
        // Đang hiển thị thời gian kết nối thẻ, tạm thời giữ nguyên màn hình LED
    }
    else if (isShowingTime)
    {
      static uint32_t lastRtcReadTime = 0;
      if (currentTick - lastRtcReadTime >= 1000 || lastRtcReadTime == 0) {
          uint8_t curH = 0, curM = 0, curS = 0;
          rtc.getTime(curH, curM, curS);
          timeValue = (curH * 100) + curM;
          lastRtcReadTime = currentTick;
      }
      
      // Mode thời gian: Nháy 1Hz bình thường không bị ảnh hưởng bởi lỗi CAN
      bool currentBlinkState = (currentTick / 500) % 2 == 0;

      if ((timeValue != lastDisplayValue) || (currentBlinkState != lastBlinkState)) {
          dp.setColon(currentBlinkState);     
          dp.setValue(timeValue);    
          dp.update(true);    
          lastDisplayValue = timeValue;
          lastBlinkState = currentBlinkState;
      }
    }
    else
    {
      // Mode đếm: Không chớp dấu chấm
      bool currentBlinkState = false;
      
      if (Sensor::productCount != lastDisplayValue || currentBlinkState != lastBlinkState) 
      {
          dp.setColon(currentBlinkState);   
          dp.setValue(Sensor::productCount);    
          dp.update(false);
          lastDisplayValue = Sensor::productCount;
          lastBlinkState = currentBlinkState;
      }
    }
    // 4: doc du lieu the
    if (currentTick - lastCardRead >= 100) {
        lastCardRead = currentTick; 
        
        // Khóa không cho quét thẻ mới nếu MP3 đang phát nhạc
        if (rfid.checkCard()) {
            if (rfid.readCardSerial((uint8_t*)uidTheHienTai)) {
                
                if (!coTheRfid || memcmp(uidTheHienTai, uidTheDaXuLy, 5) != 0) {
                    
                    // update the hien tai vao bo nho
                    memcpy(uidTheDaXuLy, uidTheHienTai, 5);
                    coTheRfid = true;
                    buzzer.beep(1, 100, 0);
                    
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
                                    if (status == STATUS_OK) buzzer.beep(1, 500, 0); 
                                    else buzzer.beep(2, 100, 100); 
                                }
                                else if (trangThaiQuetThe == 3) {
                                    uint8_t trailerData[16] = {
                                        matKhauTheMoi[0], matKhauTheMoi[1], matKhauTheMoi[2], matKhauTheMoi[3], matKhauTheMoi[4], matKhauTheMoi[5], 
                                        0xFF, 0x07, 0x80, 0x69,
                                        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF  // Key B
                                    };
                                    status = rfid.MIFARE_Write(trailerBlock, trailerData, 16);
                                    if (status == STATUS_OK) buzzer.beep(1, 500, 0);
                                    else buzzer.beep(2, 100, 100);
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
                                        if (Sensor::productCount == 0) {
                                            buzzer.beep(2, 100, 100); // Đã reset rồi, báo lỗi
                                            readStatus = 0x01;
                                        } else {
                                            if (!mp3.play("RESET.MP3")) { 
                                              uint32_t wt = SystemTimer::getTick(); 
                                              while(SystemTimer::getTick() - wt < 500) buzzer.update(); 
                                              buzzer.beep(5, 100, 100); 
                                            }
                                            // [FIX] Task 2: Cập nhật lại thanh ghi BKP khi Reset đếm
                                            Sensor::productCount = 0;
                                            BKP->DR1 = Sensor::productCount;
                                            lastDisplayValue = 0xFFFF;
                                            readStatus = 0x00;
                                            datMucTieu = false;
                                        }
                                    }
                                    else if (strncmp((char*)readData, "BAT_RELAY", 9) == 0) {
                                        if (relay.isOn()) {
                                            buzzer.beep(2, 100, 100); // Đã bật rồi, báo lỗi
                                            readStatus = 0x01;
                                        } else {
                                            if (!mp3.play("BAT.MP3")) { 
                                              uint32_t wt = SystemTimer::getTick(); 
                                              while(SystemTimer::getTick() - wt < 500) buzzer.update(); 
                                              buzzer.beep(5, 100, 100); 
                                            }
                                            dp.turnOff();
                                            relay.on();
                                            lastRelayDao = currentTick;
                                            choCapNhatManHinh = true;
                                            readStatus = 0x00;
                                        }
                                    }
                                    else if (strncmp((char*)readData, "DAO_RELAY", 9) == 0) {
                                        if (!mp3.play("DAO.MP3")) { 
                                          uint32_t wt = SystemTimer::getTick();
                                          while(SystemTimer::getTick() - wt < 500) buzzer.update(); 
                                          buzzer.beep(5, 100, 100); 
                                        }
                                        dp.turnOff();
                                        relay.toggle();
                                        lastRelayDao = currentTick;
                                        choCapNhatManHinh = true;
                                        readStatus = 0x00;
                                    }
                                    else if (strncmp((char*)readData, "PHAT_NHAC", 9) == 0) {
                                        if (!mp3.play("MUSIC.MP3")) { 
                                          uint32_t wt = SystemTimer::getTick();
                                          while(SystemTimer::getTick() - wt < 500) buzzer.update(); 
                                          buzzer.beep(5, 100, 100); 
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
    
    //6:gui du lieu can rtc vs cam bien
    if(currentTick - lastCanSend >= 500){
      uint8_t frame102[4] = {0};
      uint16_t currentCount = Sensor::productCount; 
      uint16_t currentTarget = mucTieu;
      frame102[0] = currentCount >> 8;
      frame102[1] = currentCount & 0xFF;
      
      frame102[2] = currentTarget >> 8;
      frame102[3] = currentTarget & 0xFF;
      
      myCan.sendFrame(0x102, frame102, 4);
      
      uint8_t h, m , s;
      rtc.getTime(h, m,s);
      
      uint8_t frame103[3];
      frame103[0] = ((h / 10) << 4) | (h % 10); 
      frame103[1] = ((m / 10) << 4) | (m % 10); 
      frame103[2] = ((s / 10) << 4) | (s % 10); 
      
      myCan.sendFrame(0x103, frame103, 3);
      lastCanSend = currentTick;
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
  hcan.Init.TimeSeg1 = CAN_BS1_13TQ;
  hcan.Init.TimeSeg2 = CAN_BS2_2TQ;
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

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{
  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8; 
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */
}
/**
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI2_Init(void)
{
  __HAL_RCC_SPI2_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin = GPIO_PIN_13 | GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_14;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
}

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

  GPIO_InitTypeDef GPIO_InitStruct = {0};

  // Cau hinh chan PC8
  GPIO_InitStruct.Pin = GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW; 
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_SET);
  
  // Cau hinh chan PB1 
  GPIO_InitStruct.Pin = GPIO_PIN_1;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET);
  
  //Cau hinh PA4
  GPIO_InitStruct.Pin = GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET); // Khởi động mặc định ở trạng thái Tắt Amply (Mute) để triệt tiêu tiếng bụp
  
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
                if (isValidBCD(rxData[0]) && isValidBCD(rxData[1])) {
                    uint16_t val = (BCD_to_Dec(rxData[0]) * 100) + BCD_to_Dec(rxData[1]);

                    if (val <= 9999) {
                        mucTieu = val;
                        datMucTieu = false;
                        buzzer.beep(1, 150, 0);
                    }else buzzer.beep(2, 100, 100);
                }else buzzer.beep(2, 100, 100);
            }
            //0x202 dem hien tai
            else if (rxId == 0x202) {
                uint16_t val = (rxData[0] << 8) | rxData[1];
                if (val <= 9999) {
                    // [FIX] Task 2: Cập nhật lại thanh ghi BKP khi set giá trị đếm từ mạng CAN
                    Sensor::productCount = val;
                    BKP->DR1 = Sensor::productCount;
                    datMucTieu = false;
                    buzzer.beep(1, 150, 0);
                } else {
                    buzzer.beep(2, 100, 100);
                }
            }
            //0x203 time
            else if (rxId == 0x203) {
                if (isValidBCD(rxData[0]) && isValidBCD(rxData[1]) && isValidBCD(rxData[2])) {
                    uint8_t h = BCD_to_Dec(rxData[0]);
                    uint8_t m = BCD_to_Dec(rxData[1]);
                    uint8_t s = BCD_to_Dec(rxData[2]);
                    if (h <= 23 && m <= 59 && s <= 59) {
                        rtc.setTime(h, m, s);
                        buzzer.beep(1, 150, 0);
                    }else buzzer.beep(2, 100, 100);
                }else buzzer.beep(2, 100, 100);
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









