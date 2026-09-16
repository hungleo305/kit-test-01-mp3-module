#include "Mp3.h"
#include <string.h>
#include "stm32f1xx_hal.h"
#include "BootSound.h"

/* ==================== Địa chỉ thanh ghi SCI ==================== */
#define SCI_MODE        0x00
#define SCI_STATUS      0x01
#define SCI_BASS        0x02
#define SCI_CLOCKF      0x03
#define SCI_DECODE_TIME 0x04
#define SCI_AUDATA      0x05
#define SCI_WRAM        0x06
#define SCI_WRAMADDR    0x07
#define SCI_HDAT0       0x08
#define SCI_HDAT1       0x09
#define SCI_AIADDR      0x0A
#define SCI_VOL         0x0B
#define SCI_AICTRL0     0x0C

/* ==================== Bit điều khiển SCI_MODE ==================== */
#define SM_RESET        0x0004
#define SM_SDINEW       0x0800
#define SM_STREAM       0x0040
#define SM_CANCEL       0x0008

#define BOARD_XTAL_HZ    12000000UL

/* ==================== File MP3 đang mở ==================== */
static FIL mp3File;

/* ==================== Constructor ==================== */
Mp3::Mp3(SpiBus* spi_bus,
         GPIO_TypeDef* pwrPort,    uint16_t pwrPin,
         GPIO_TypeDef* xresetPort, uint16_t xresetPin,
         GPIO_TypeDef* dreqPort,   uint16_t dreqPin,
         GPIO_TypeDef* xdcsPort,   uint16_t xdcsPin,
         GPIO_TypeDef* xcsPort,    uint16_t xcsPin)
    : spi_bus(spi_bus),
      pwrPort(pwrPort),       pwrPin(pwrPin),
      xresetPort(xresetPort), xresetPin(xresetPin),
      dreqPort(dreqPort),     dreqPin(dreqPin),
      xdcsPort(xdcsPort),     xdcsPin(xdcsPin),
      xcsPort(xcsPort),       xcsPin(xcsPin),
      isPlaying(false),
      hasSavedState(false),
      playSpeed(1.0f),
      bufferIndex(0),
      dataInBuf(0),
      queueHead(0), queueTail(0), queueCount(0)
{
    memset(currentFilename, 0, sizeof(currentFilename));
    memset(savedFilename,   0, sizeof(savedFilename));
    savedPosition = 0;
}

/* ==================== Chờ chip sẵn sàng (DREQ HIGH) ==================== */
bool Mp3::waitReady() {
    uint32_t timeout = HAL_GetTick();
    while ((dreqPort->IDR & dreqPin) == 0) {
        if (HAL_GetTick() - timeout > 500) {
            return false;
        }
    }
    return true;
}

/* ==================== Ghi thanh ghi SCI ==================== */
void Mp3::writeRegister(uint8_t address, uint16_t data) {
    if (!waitReady()) return;

    xcsPort->BRR = xcsPin;

    uint8_t cmd[4];
    cmd[0] = 0x02;                          // Opcode ghi
    cmd[1] = address;
    cmd[2] = (data >> 8) & 0xFF;
    cmd[3] =  data       & 0xFF;

    spi_bus->TransferBuffer(cmd, nullptr, 4);

    xcsPort->BSRR = xcsPin;
}

/* ==================== Đọc thanh ghi SCI ==================== */
uint16_t Mp3::readRegister(uint8_t address) {
    if (!waitReady()) return 0;

    xcsPort->BRR = xcsPin;

    uint8_t tx[4] = {0x03, address, 0xFF, 0xFF};
    uint8_t rx[4] = {0, 0, 0, 0};

    spi_bus->TransferBuffer(tx, rx, 4);

    xcsPort->BSRR = xcsPin;

    return (rx[2] << 8) | rx[3];
}

/* ==================== Chuyển hệ số tốc độ -> giá trị CLOCKF ==================== */
uint16_t Mp3::speedToClockf(float speed) {
    if (speed < 0.2f) speed = 0.2f;
    if (speed > 5.0f) speed = 5.0f;

    // Công thức tính chuẩn của VS1011e:
    // Bits 14:0 = XTALI / 2000
    // Để đánh lừa chip chạy nhanh hơn, ta báo cho chip biết XTALI NHỎ HƠN thực tế
    uint32_t actualXtal = 12000000; // Thạch anh thực tế 12MHz
    uint32_t fakeXtal = (uint32_t)(actualXtal / speed);
    uint16_t sc_freq = (uint16_t)(fakeXtal / 2000);

    // Bit 15: Bật Clock Doubler (0x8000)
    // Bits 14:0: Tần số SC_FREQ
    return 0x8000 | (sc_freq & 0x7FFF);
}

/* ==================== Reset chip ==================== */
void Mp3::reset() {
    // Reset cứng
    xresetPort->BRR = xresetPin;
    HAL_Delay(20);
    xresetPort->BSRR = xresetPin;
    HAL_Delay(20);

    // Bật SDI mới + soft reset
    writeRegister(SCI_MODE, SM_SDINEW | SM_RESET);
    HAL_Delay(20);
    waitReady();

    // Đặt tốc độ mặc định 1.0x
    setPlaySpeed(1.0f);

    setVolume(40, 40);

    isPlaying    = false;
    bufferIndex  = 0;
    dataInBuf    = 0;
}

/* ==================== Khởi tạo chân và chip ==================== */
void Mp3::init() {
    spi_bus->Init();

    // Cấu hình 4 chân output: PWR, XRESET, XDCS, XCS
    GPIO_TypeDef* ports[] = {pwrPort, xresetPort, xdcsPort, xcsPort};
    uint16_t      pins[]  = {pwrPin,  xresetPin,  xdcsPin,  xcsPin};

    for (int i = 0; i < 4; i++) {
        // Bật clock cho GPIO port
        if      (ports[i] == GPIOA) RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
        else if (ports[i] == GPIOB) RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
        else if (ports[i] == GPIOC) RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
        else if (ports[i] == GPIOD) RCC->APB2ENR |= RCC_APB2ENR_IOPDEN;

        // Tìm chỉ số bit của chân
        uint32_t pinIndex = 0;
        while (!(pins[i] & (1 << pinIndex))) pinIndex++;

        // Cấu hình output push-pull 50MHz
        if (pinIndex < 8) {
            ports[i]->CRL &= ~(0xF << (pinIndex * 4));
            ports[i]->CRL |=  (0x3 << (pinIndex * 4));
        } else {
            ports[i]->CRH &= ~(0xF << ((pinIndex - 8) * 4));
            ports[i]->CRH |=  (0x3 << ((pinIndex - 8) * 4));
        }
    }

    // Cấu hình DREQ làm input floating (chip điều khiển chân này)
    uint32_t pinIndex = 0;
    while (!(dreqPin & (1 << pinIndex))) pinIndex++;
    if (pinIndex < 8) {
        dreqPort->CRL &= ~(0xF << (pinIndex * 4));
        dreqPort->CRL |=  (0x4 << (pinIndex * 4));
    } else {
        dreqPort->CRH &= ~(0xF << ((pinIndex - 8) * 4));
        dreqPort->CRH |=  (0x4 << ((pinIndex - 8) * 4));
    }

    // Đặt mức mặc định
    xcsPort->BSRR    = xcsPin;      // XCS HIGH
    xdcsPort->BSRR   = xdcsPin;     // XDCS HIGH
    xresetPort->BSRR = xresetPin;   // XRESET HIGH
    pwrPort->BRR     = pwrPin;      // PWR LOW -> PNP mở -> cấp 3V3

    HAL_Delay(100);                 // Chờ nguồn ổn định
    uint8_t dummy = 0xFF;
    spi_bus->Transfer(dummy);       // Xung SPI đầu tiên để wake chip

    reset();
    isPlaying = false;
}

/* ==================== Điều chỉnh âm lượng ==================== */
void Mp3::setVolume(uint8_t left, uint8_t right) {
    uint16_t v = (left << 8) | right;
    writeRegister(SCI_VOL, v);
}

/* ==================== ĐẶT TỐC ĐỘ PHÁT QUA THẠCH ANH ==================== */
//
// Phương pháp:
//   - Thay vì ép tần số lấy mẫu (SCI_AUDATA), ta thay đổi hệ số nhân clock
//     nội bộ của chip VS1011E thông qua thanh ghi SCI_CLOCKF.
//   - Chip chạy DSP nhanh/chậm hơn -> toàn bộ chuỗi xử lý (giải mã, DAC)
//     chạy nhanh/chậm theo -> audio phát nhanh/chậm.
//   - CẦN LƯU Ý: cách này làm thay đổi CAO ĐỘ (pitch) tỉ lệ với tốc độ.
//     Nếu muốn giữ nguyên pitch, cần phương pháp time-stretching khác.
//
bool Mp3::setPlaySpeed(float speedMultiplier) {
    // Giới hạn dải hợp lệ
    if (speedMultiplier < 0.2f) speedMultiplier = 0.2f;
    if (speedMultiplier > 5.0f) speedMultiplier = 5.0f;
    
    playSpeed = speedMultiplier;

    // Bước 1: Lấy giá trị CLOCKF tương ứng
    uint16_t newClockf = speedToClockf(speedMultiplier);

    // Bước 2: Ghi trực tiếp SCI_CLOCKF của VS1011e
    writeRegister(SCI_CLOCKF, newClockf);

    // Bước 3: Chip cần thời gian khóa lại clock mới
    uint16_t currentMode = readRegister(SCI_MODE);
    writeRegister(SCI_MODE, currentMode | SM_RESET);
    HAL_Delay(10);
    waitReady();

    return true;
}

/* ==================== Thêm file vào hàng đợi ==================== */
bool Mp3::play(const char* filename) {
    // Kiểm tra file có tồn tại không trước khi xếp hàng
    static FIL tempFile;
    FRESULT fres = f_open(&tempFile, filename, FA_READ);
    if (fres != FR_OK) return false;

    f_close(&tempFile);

    if (queueCount >= 5) return false;

    strncpy(queue[queueTail], filename, 31);
    queue[queueTail][31] = '\0';
    queueTail = (queueTail + 1) % 5;
    queueCount++;

    return true;
}

/* ==================== Vòng lặp chính, gọi liên tục từ main ==================== */
void Mp3::process() {
    /* ---------- 1. Mở file mới nếu chưa phát ---------- */
    if (!isPlaying) {
        if (queueCount > 0) {
            FRESULT fres = f_open(&mp3File, queue[queueHead], FA_READ);

            if (fres == FR_OK) {
                strncpy(currentFilename, queue[queueHead], 31);
                currentFilename[31] = '\0';

                bufferIndex = 0;
                dataInBuf   = 0;
                isPlaying   = true;
            }

            queueHead = (queueHead + 1) % 5;
            queueCount--;
        }
        return;
    }

    /* ---------- 2. Bơm dữ liệu MP3 sang chip VS1011E ---------- */
    int chunksSent = 0;

    while (((dreqPort->IDR & dreqPin) ? 1 : 0) == 1 && chunksSent < 64) {

        // Nếu bộ đệm đã gửi hết, đọc thêm từ SD
        if (bufferIndex >= dataInBuf) {
            UINT bytesRead;
            FRESULT fres = f_read(&mp3File, audioBuffer, sizeof(audioBuffer), &bytesRead);

            dataInBuf   = bytesRead;
            bufferIndex = 0;

            if (bytesRead == 0) {
                // Lỗi đọc (thẻ bị rút) -> lưu trạng thái để phát tiếp
                if (fres != FR_OK) {
                    saveState();
                    return;
                }

                // Hết file -> đóng và gửi padding để chip xả hết buffer
                f_close(&mp3File);
                isPlaying = false;

                uint8_t endFill[32] = {0};

                // Gửi 65 khối padding (theo khuyến nghị VLSI)
                for (int i = 0; i < 65; i++) {
                    if (!waitReady()) break;
                    xdcsPort->BRR = xdcsPin;
                    spi_bus->TransferDMA(endFill, nullptr, 32);
                    xdcsPort->BSRR = xdcsPin;
                }

                // Bật SM_CANCEL để chip dừng sạch
                uint16_t mode = readRegister(SCI_MODE);
                mode |= SM_CANCEL;
                writeRegister(SCI_MODE, mode);

                // Chờ chip xác nhận đã hủy xong
                for (int i = 0; i < 64; i++) {
                    if (!waitReady()) break;
                    xdcsPort->BRR = xdcsPin;
                    spi_bus->TransferDMA(endFill, nullptr, 32);
                    xdcsPort->BSRR = xdcsPin;

                    mode = readRegister(SCI_MODE);
                    if (!(mode & SM_CANCEL)) break;
                }

                HAL_Delay(50);
                return;
            }
        }

        // Gửi 1 khối tối đa 32 byte
        uint16_t chunk = dataInBuf - bufferIndex;
        if (chunk > 32) chunk = 32;

        xdcsPort->BRR = xdcsPin;
        spi_bus->TransferDMA(&audioBuffer[bufferIndex], nullptr, chunk);
        xdcsPort->BSRR = xdcsPin;

        bufferIndex += chunk;
        chunksSent++;
    }
}

/* ==================== Phát sine test ==================== */
void Mp3::testAudio() {
    writeRegister(SCI_MODE, 0x0820);   // SM_TESTS | SM_SDINEW
    HAL_Delay(50);

    uint8_t sineCmd[8]    = {0x53, 0xEF, 0x6E, 0x44, 0x00, 0x00, 0x00, 0x00};
    uint8_t exitCmd[8]    = {0x45, 0x78, 0x69, 0x74, 0x00, 0x00, 0x00, 0x00};

    // Gửi lệnh sine
    waitReady();
    xdcsPort->BRR = xdcsPin;
    spi_bus->TransferBuffer(sineCmd, nullptr, 8);
    xdcsPort->BSRR = xdcsPin;

    HAL_Delay(2000);

    // Gửi lệnh thoát
    waitReady();
    xdcsPort->BRR = xdcsPin;
    spi_bus->TransferBuffer(exitCmd, nullptr, 8);
    xdcsPort->BSRR = xdcsPin;

    writeRegister(SCI_MODE, SM_SDINEW);
}

/* ==================== Phát âm thanh khởi động ==================== */
void Mp3::playBootSound() {
    // Lưu tốc độ hiện tại và ép về 1.0 để âm thanh khởi động không bị nhanh/chậm
    float oldSpeed = this->playSpeed;
    setPlaySpeed(1.0f);

    uint32_t remaining = bootSoundLength;
    uint32_t offset = 0;

    // Khôi phục về chế độ bình thường (không dùng SM_TESTS)
    writeRegister(SCI_MODE, SM_SDINEW);
    HAL_Delay(10);

    // Bơm dữ liệu file WAV 1s (trong mảng BootSound) vào chip
    while (remaining > 0) {
        if (!waitReady()) break;

        uint16_t chunk = (remaining > 32) ? 32 : remaining;
        
        xdcsPort->BRR = xdcsPin;
        spi_bus->TransferBuffer((uint8_t*)&bootSoundArray[offset], nullptr, chunk);
        xdcsPort->BSRR = xdcsPin;

        offset += chunk;
        remaining -= chunk;
    }

    // Gửi padding để VS1011e xả hết buffer âm thanh ra DAC
    uint8_t endFill[32] = {0};
    for (int i = 0; i < 65; i++) {
        if (!waitReady()) break;
        xdcsPort->BRR = xdcsPin;
        spi_bus->TransferBuffer(endFill, nullptr, 32);
        xdcsPort->BSRR = xdcsPin;
    }

    // Khôi phục lại tốc độ cũ (như cài đặt ở main)
    setPlaySpeed(oldSpeed);
}

/* ==================== Lưu trạng thái khi rút thẻ ---------- */
void Mp3::saveState() {
    if (isPlaying) {
        strncpy(savedFilename, currentFilename, 31);
        savedFilename[31] = '\0';
        savedPosition     = f_tell(&mp3File);
        hasSavedState     = true;

        f_close(&mp3File);
        isPlaying = false;
    }
}

/* ==================== Khôi phục trạng thái khi cắm lại thẻ ---------- */
void Mp3::resumeState() {
    if (hasSavedState) {
        FRESULT fres = f_open(&mp3File, savedFilename, FA_READ);

        if (fres == FR_OK) {
            f_lseek(&mp3File, savedPosition);

            bufferIndex = 0;
            dataInBuf   = 0;
            isPlaying   = true;

            strncpy(currentFilename, savedFilename, 31);
            currentFilename[31] = '\0';
        }

        hasSavedState = false;
    }
}