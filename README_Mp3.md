# Sổ Tay Toàn Tập Thư Viện Giải Mã Âm Thanh Mp3 (VS1011e)

Phần này hướng dẫn sử dụng thư viện `Mp3` để giao tiếp với chip giải mã âm thanh VS1011e thông qua giao thức truyền dẫn SPI.

---

## PHẦN 1: CHI TIẾT TỪNG HÀM LỆNH API THƯ VIỆN MP3

### 1.1 Nhóm Khởi tạo & Trạng thái

**`Mp3(SpiBus* spi_bus, ...)`**
- **Chức năng:** Khởi tạo thư viện và ánh xạ các chân điều khiển phần cứng của mạch vào chip VS1011e (PWR, XRESET, DREQ, XDCS, XCS).

**`void init()`**
- **Chức năng:** Khởi động chip VS1011e, bật nguồn cấp, gửi tín hiệu reset cứng và mồi xung SPI đầu tiên. **Bắt buộc** gọi 1 lần ở đầu chương trình.

**`void reset()`**
- **Chức năng:** Gửi lệnh Soft Reset (`SM_RESET`) qua giao thức SPI tới chip để xóa bộ đệm, khôi phục chip về trạng thái chuẩn, sẵn sàng nhận luồng âm thanh mới.

### 1.2 Nhóm Điều khiển Âm thanh

**`void setVolume(uint8_t left, uint8_t right)`**
- **Chức năng:** Điều chỉnh âm lượng cho 2 kênh trái/phải. Giá trị quy định của chip là từ `0` (To nhất) đến `254` (Im lặng).
- **Kinh nghiệm:** Để ở mức `40` đến `70` là tốt nhất, vừa đủ to mà không bị rè do quá ngưỡng công suất của Amply.

**`bool setPlaySpeed(float speedMultiplier)`**
- **Chức năng:** Căn chỉnh tốc độ phát âm thanh (từ `0.2x` đến `5.0x`) bằng cách can thiệp trực tiếp vào cấu hình tần số thạch anh (`SCI_CLOCKF`) của chip. Bằng cách đánh lừa chip rằng thạch anh đang chạy chậm lại, chip sẽ tự động tăng tốc độ lấy mẫu DSP lên.
- **Lưu ý:** Việc thay đổi tốc độ ở cấp độ phần cứng theo cách này sẽ làm biến đổi cao độ (Pitch). Tốc độ càng nhanh giọng sẽ càng quéo lại giống sóc chuột.

### 1.3 Nhóm Phát Nhạc

**`bool play(const char* filename)`**
- **Chức năng:** Ra lệnh mở file MP3 từ thẻ nhớ và nạp vào danh sách chờ. Nếu chip đang rảnh, nó sẽ phát ngay lập tức.
- **Lưu ý:** Lệnh này chỉ thực hiện mở file. Nhạc chỉ thực sự được phát ra loa nếu bạn liên tục gọi hàm `process()` trong vòng lặp chính.

**`void process()`**
- **Chức năng:** Hàm xử lý luồng dữ liệu chính của thư viện MP3. Hàm này cần được gọi liên tục trong vòng lặp `while(1)`. Nó chịu trách nhiệm kiểm tra trạng thái chân `DREQ`, đọc từng khối `32 bytes` từ file trên thẻ nhớ và gửi vào chip giải mã. Hàm cũng tự động chuyển bài nếu có bài hát mới trong danh sách chờ.

**`void playBootSound()`**
- **Chức năng:** Phát âm thanh trực tiếp từ mảng dữ liệu C-array lưu trong bộ nhớ Flash (như `BootSound.h`), không phụ thuộc vào thẻ nhớ SD. Thường được sử dụng để phát âm báo khởi động hoặc các cảnh báo hệ thống ngắn.
- **Lưu ý:** Hàm này hoạt động theo cơ chế chặn (Blocking). Vi điều khiển sẽ dừng các tiến trình khác cho đến khi việc truyền mảng âm thanh vào chip hoàn tất.

### 1.4 Nhóm Quản lý Tạm dừng / Tháo thẻ

**`void saveState()`**
- **Chức năng:** Lưu lại trạng thái bài nhạc đang phát dở (tên file hiện hành và vị trí giây/phút - con trỏ file) vào một file tạm. Sử dụng khi phát hiện chân ngắt (SD Detect) báo hiệu thẻ nhớ bị người dùng rút ra đột ngột.

**`void resumeState()`**
- **Chức năng:** Đọc lại file tạm lưu trữ ở trên và tua con trỏ file về đúng vị trí cũ, giúp bản nhạc tự động phát tiếp một cách mượt mà ngay khi người dùng cắm thẻ trở lại.

---

## PHẦN 2: HƯỚNG DẪN SỬ DỤNG VÀ VÍ DỤ CƠ BẢN

### 2.1 Khởi tạo và Mẹo Bật Amply Ngoại (Quan trọng)
Trong thiết kế phần cứng của bộ Kit này, có một tầng khuếch đại (Amply ngoài) can thiệp trước khi ra loa. **Chân PA4** điều khiển trạng thái Mute (im lặng) của Amply này. Bắt buộc phải kéo chân PA4 xuống mức 0 thì loa mới phát ra tiếng.

```cpp
// 1. Khai báo Bus SPI (Dành riêng cho giao tiếp với chip MP3)
SpiBus spi2_bus(SPI2); 

// 2. Khai báo Đối tượng MP3 theo sơ đồ nối chân
Mp3 mp3(&spi2_bus, GPIOC, GPIO_PIN_14, /* PWR */
                   GPIOB, GPIO_PIN_11, /* XRESET */
                   GPIOB, GPIO_PIN_10, /* DREQ */
                   GPIOA, GPIO_PIN_15, /* XDCS */
                   GPIOA, GPIO_PIN_8); /* XCS */

void setup() {
    mp3.init();
    mp3.setPlaySpeed(1.0f); // Tốc độ chuẩn 1.0x
    mp3.setVolume(70, 70);  // Âm lượng vừa đủ nghe

    // 3. Phát âm thanh khởi động từ bộ nhớ nội (ROM)
    GPIOA->BRR = (1 << 4);  // Bật Amply ngoài (Unmute chân PA4)
    mp3.playBootSound();
    GPIOA->BSRR = (1 << 4); // Tắt Amply lại để chống tiếng sôi nhiễu (xẹt xẹt) trong lúc chờ khởi tạo thẻ SD
}
```

### 2.2 Vòng lặp phát nhạc đa nhiệm
Sau khi thẻ nhớ đã sẵn sàng, bạn có thể gọi lệnh `mp3.play()` để đưa bài vào danh sách. Việc nhồi dữ liệu thực tế do `mp3.process()` gánh vác trong hàm loop.

```cpp
void loop() {
    // 1. Luôn ưu tiên nhồi dữ liệu nếu DREQ của chip MP3 báo rảnh
    mp3.process();

    // 2. Kích hoạt phát nhạc khi có sự kiện (ví dụ: quét thẻ RFID)
    if (quetTheThanhCong()) {
        GPIOA->BRR = (1 << 4);    // Bật Amply lên
        mp3.play("XINCHAO.MP3"); 
    }
}
```

---

## PHẦN 3: HƯỚNG DẪN TẠO VÀ PHÁT ÂM THANH TỪ FLASH (KHÔNG DÙNG THẺ NHỚ)

Thư viện hỗ trợ phát trực tiếp âm thanh từ bộ nhớ Flash của vi điều khiển (thông qua hàm `playBootSound()`), rất hữu ích cho các file âm thanh ngắn (chuông báo, tiếng bip) mà không cần sử dụng hệ thống tệp thẻ nhớ. Dữ liệu âm thanh MP3/WAV cần được chuyển đổi thành mảng C/C++.

### 3.1 Chuyển đổi file MP3 sang file Header (C-array)
Dự án đã có sẵn một script Python (`convert_mp3.py`) để tự động hóa quá trình này.

1. Đặt file âm thanh (ví dụ: `khoi_dong.mp3`) vào cùng thư mục với script.
2. Mở terminal và chạy lệnh:
   ```bash
   python convert_mp3.py khoi_dong.mp3
   ```
3. Script sẽ tự động biên dịch file MP3 và tạo ra mảng byte lưu trong file `Core/Inc/BootSound.h`.
   *(Lưu ý: Kích thước file gốc nên được nén tối đa hoặc cắt ngắn để không làm tràn dung lượng Flash giới hạn của vi điều khiển).*

### 3.2 Tích hợp mảng âm thanh vào hàm thư viện
Trong file `Core/Src/Mp3.cpp`, hãy đảm bảo đã `#include "BootSound.h"` ở đầu file. Bên trong hàm `playBootSound()`, dữ liệu sẽ được đọc trực tiếp từ mảng `bootSoundArray`:

```cpp
void Mp3::playBootSound() {
    // 1. Đồng bộ tốc độ về 1.0x để đảm bảo cao độ chuẩn
    float oldSpeed = this->playSpeed;
    setPlaySpeed(1.0f);

    uint32_t remaining = bootSoundLength; // Lấy từ file BootSound.h
    uint32_t offset = 0;

    writeRegister(SCI_MODE, SM_SDINEW);
    HAL_Delay(10);

    // 2. Bơm mảng dữ liệu từ Flash vào VS1011E
    while (remaining > 0) {
        if (!waitReady()) break;

        uint16_t chunk = (remaining > 32) ? 32 : remaining;
        
        xdcsPort->BRR = xdcsPin;
        spi_bus->TransferBuffer((uint8_t*)&bootSoundArray[offset], nullptr, chunk);
        xdcsPort->BSRR = xdcsPin;

        offset += chunk;
        remaining -= chunk;
    }

    // 3. Padding xả buffer nội bộ
    uint8_t endFill[32] = {0};
    for (int i = 0; i < 65; i++) {
        if (!waitReady()) break;
        xdcsPort->BRR = xdcsPin;
        spi_bus->TransferBuffer(endFill, nullptr, 32);
        xdcsPort->BSRR = xdcsPin;
    }

    // Khôi phục tốc độ cũ
    setPlaySpeed(oldSpeed);
}
```

### 3.3 Gọi hàm phát âm thanh trong main
Để phát âm thanh ngay lập tức sau khi mạch cấp nguồn, hãy gọi hàm ở khối khởi tạo hệ thống trong `main.cpp`, chú ý cấp lệnh ngắt Mute (chân PA4) tương ứng với phần cứng:

```cpp
  mp3.init();
  mp3.setPlaySpeed(1.0f);
  mp3.setVolume(70,70);
  
  GPIOA->BRR = (1 << 4); // Bật Amply (Kéo chân PA4 xuống 0)
  mp3.playBootSound(); 
  GPIOA->BSRR = (1 << 4); // Tắt lại Amply chờ khởi tạo thiết bị khác
```

---

## PHẦN 4: LƯU Ý KỸ THUẬT VÀ TỐI ƯU HÓA CHO CHIP VS1011E

1. **Cảnh báo khi can thiệp thanh ghi tần số (SCI_CLOCKF):**
   Mặc dù thư viện lợi dụng thanh ghi `SCI_CLOCKF` để "đánh lừa" chip nhằm thay đổi tốc độ phát (trong hàm `setPlaySpeed`), bạn **tuyệt đối không được** thiết lập các bit ở vùng cao (D15..D13) thành hệ số nhân (Clock Multiplier) theo thói quen dùng chip VS1053. Chip VS1011E không hỗ trợ hệ số nhân phần cứng; nếu vô tình nạp các bit này, bộ giải mã DAC sẽ bị sập, phát ra tiếng rít chói tai hoặc mất hẳn âm thanh. Chỉ được phép thay đổi cụm bit giá trị `(XTALI_HZ / 2000)` để ép chip tăng/giảm tốc độ lấy mẫu.

2. **Quản lý chân DREQ (Data Request):**
   Chân DREQ báo hiệu trạng thái sẵn sàng nhận dữ liệu của chip VS1011E. Vi điều khiển STM32 chỉ được phép gửi dữ liệu (hoặc lệnh) khi chân `DREQ` ở mức CAO (1). Việc tiếp tục gửi dữ liệu khi `DREQ = 0` sẽ dẫn đến tràn bộ đệm nội bộ của chip, làm mất dữ liệu MP3 và sinh ra nhiễu hoặc giật tiếng.

3. **Kiểm soát cờ báo SPI bận (SPI BSY Flag):**
   Khi gửi một mảng dữ liệu qua khối SPI của STM32, CPU thường thực thi xong các lệnh trước khi bộ phần cứng SPI hoàn tất việc dịch bit ra ngoài. Nếu CPU kéo chân cấp phép (XDCS) lên mức CAO trong khi phần cứng SPI vẫn đang truyền bit cuối cùng (cờ BSY = 1), khung truyền dữ liệu sẽ bị ngắt quãng. Điều này làm sai lệch cấu trúc dữ liệu khiến VS1011E ngừng giải mã. Cần sử dụng vòng lặp kiểm tra cờ `SPI_SR_BSY` trở về 0 trước khi giải phóng chân XDCS.

4. **Kỹ thuật xả Buffer (EndFillBytes):**
   Sau khi gửi xong byte dữ liệu cuối cùng của bài hát, bên trong RAM nội bộ của chip VS1011e vẫn còn chứa khoảng 2048 byte dữ liệu cũ chưa kịp đẩy ra bộ giải mã DAC. Nếu tắt chân kích Amply (PA4) ngay lập tức lúc này, đoạn cuối âm thanh sẽ bị cắt cụt. 
   **Khắc phục:** Cần gửi thêm 65 khối (khoảng `2048` byte) dữ liệu giả `0x00` để đẩy toàn bộ âm thanh thừa trong buffer ra loa, đảm bảo tín hiệu âm thanh hoàn tất trước khi tắt Amply.
