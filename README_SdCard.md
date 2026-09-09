# Sổ Tay Toàn Tập Thư Viện SdCard và FatFs (C++)

Phần này hướng dẫn sử dụng thư viện giao tiếp Thẻ nhớ MicroSD (`SdCard`) và cách kết hợp với thư viện hệ thống tệp tin (`FatFs`).

---

## PHẦN 1: CHI TIẾT TỪNG HÀM LỆNH API THƯ VIỆN SDCARD

Thư viện `SdCard` là lớp trung gian kết nối phần cứng SPI của STM32 với thư viện hệ thống tệp **FatFs**. Dưới đây là giải thích rõ toàn bộ các hàm được cung cấp bởi lớp này.

### 1.1 Nhóm Khởi tạo & Quản lý Nguồn

**`SdCard(SpiBus* spi_bus, GPIO_TypeDef* csPort, uint16_t csPin, GPIO_TypeDef* cdPort, uint16_t cdPin, GPIO_TypeDef* pwrPort, uint16_t pwrPin)`**
- **Chức năng:** Hàm khởi tạo (Constructor).
- **Tham số:** 
  - `spi_bus`: Con trỏ đến đối tượng `SpiBus` đã được cấu hình từ trước.
  - Các cặp Port/Pin cho `CS` (Chip Select - Kích hoạt chip), `CD` (Card Detect - Chân báo cắm thẻ), và `PWR` (Power - Chân điều khiển bật/tắt nguồn cho thẻ).

**`void init()`**
- **Chức năng:** Tự động kích hoạt xung nhịp Clock và thiết lập cấu hình GPIO cho các chân CS, CD, PWR ở trạng thái an toàn mặc định. **Bắt buộc** gọi 1 lần ở đầu chương trình.

**`bool checkPhysicalPresence()`**
- **Chức năng:** Đọc trạng thái chân CD (Card Detect). Trả về `true` nếu thẻ đang nằm vật lý trong khay, `false` nếu khay trống. Rất hữu ích để làm tính năng "Hot-plug" (Rút cắm nóng, tự động lưu file khi rút thẻ).

**`void powerOn()` / `void powerOff()`**
- **Chức năng:** Đóng ngắt chân PWR để cấp hoặc cắt nguồn điện 3.3V cho mạch thẻ nhớ. Dùng để tiết kiệm pin hoặc Reset cứng thẻ SD khi nó bị treo.

### 1.2 Nhóm Giao tiếp tầng thấp (Disk I/O)
Các hàm này thường **KHÔNG** được gọi trực tiếp trong code ứng dụng (hàm main). Thư viện hệ thống **FatFs** sẽ tự động gọi ngầm.

**`uint8_t initialize()`**
- Khởi động chuỗi lệnh chuẩn bị (CMD0, CMD8, CMD55, ACMD41...) để ép thẻ SD bỏ giao thức SDIO và chuyển sang chế độ SPI mode. Tự động chỉnh tốc độ SPI sang mức thấp (Low Speed) lúc mồi, rồi đẩy lên tốc độ cao (High Speed) lúc chạy.

**`uint8_t read(uint8_t *buff, uint32_t sector, uint32_t count)`**
- Đọc `count` block dữ liệu thô (mỗi block luôn là 512 byte) bắt đầu từ `sector` vật lý trên thẻ, lưu vào mảng đệm `buff`.

**`uint8_t write(const uint8_t *buff, uint32_t sector, uint32_t count)`**
- Ghi dữ liệu thô từ `buff` xuống chip Flash của thẻ nhớ. Tương tự như hàm read, thao tác luôn thực hiện theo đơn vị Block 512 byte.

**`uint8_t status()`**
- Trả về trạng thái hiện tại của thẻ nhớ (Ví dụ: Thẻ đang hoạt động, Không có thẻ, Thẻ bị cấm ghi...). FatFs gọi hàm này định kỳ để biết người dùng có rút thẻ ra đột ngột hay không.

**`uint8_t ioctl(uint8_t cmd, void *buff)`**
- Hàm I/O Control, dùng để cung cấp các thông số cấu hình thẻ cho FatFs hoặc ra lệnh đặc biệt. 
- Trong dự án này, nó đảm nhận hai tác vụ chính khi FatFs yêu cầu:
  - Báo cáo số lượng Sector và Sector Size (512 Bytes) để FatFs biết cách phân định dung lượng ổ đĩa.
  - Bắt ép thẻ nhớ phải lưu xong xuôi dữ liệu xuống Flash (Lệnh `CTRL_SYNC`) trước khi cho phép hàm `f_close()` đóng file, ngăn chặn việc mất dữ liệu do tắt nguồn.

### 1.3 Nhóm Hàm Trợ Lý Ẩn (Private Helpers)
Đây là các hàm nội bộ (`private`) được thư viện sử dụng ngầm bên dưới để duy trì giao tiếp chuẩn mực với chip Flash của thẻ. Mặc dù không thể gọi chúng từ bên ngoài, việc hiểu chúng sẽ giúp nắm rõ nguyên lý hoạt động của thẻ SD.

**`void setSpiSpeedSlow()` / `void setSpiSpeedFast()`**
- **Chức năng:** Thẻ SD khi mới cấp điện bắt buộc phải được mồi bằng giao tiếp tốc độ rất chậm (thường từ 100kHz đến 400kHz). Sau khi thẻ phản hồi thành công và khởi động xong chip Flash nội bộ, MCU mới được phép kích tốc độ SPI lên tối đa (như 18MHz) bằng hàm `setSpiSpeedFast()`.

**`void csLow()` / `void csHigh()`**
- **Chức năng:** Điều khiển chân `CS` (Chip Select). 
  - `csLow()`: Kéo chân CS xuống mức 0V để báo cho thẻ biết "Hãy lắng nghe, MCU chuẩn bị gửi lệnh".
  - `csHigh()`: Kéo chân CS lên 3.3V để giải phóng thẻ. **Quan trọng:** Giao thức SPI yêu cầu trước và sau khi gạt chân CS, MCU luôn phải bơm thêm một byte giả (`0xFF` - DUMMY BYTE) để cung cấp đủ xung nhịp Clock cho thẻ xử lý nốt dữ liệu còn tồn đọng. 

**`uint8_t waitReady()`**
- **Chức năng:** Chờ thẻ xử lý xong tác vụ. Khi thẻ đang bận (ví dụ: đang lưu dữ liệu vào Flash), nó sẽ kéo đường dây MISO xuống mức LOW. Hàm này liên tục bơm Dummy Byte và đọc lại MISO cho đến khi đường dây được nhả lên mức HIGH (0xFF) hoặc bị quá thời gian (Timeout 500ms chống treo hệ thống).

**`uint8_t sendCmd(uint8_t cmd, uint32_t arg)`**
- **Chức năng:** Cấu trúc thành một gói lệnh chuẩn (Packet) dài 6 bytes để gửi cho thẻ. Bao gồm: 
  - 1 Byte mã lệnh (`CMDx | 0x40`)
  - 4 Bytes tham số (`arg`)
  - 1 Byte mã kiểm tra lỗi CRC. (Giao thức SPI mặc định tắt kiểm tra CRC, nhưng CMD0 và CMD8 bắt buộc phải có CRC cứng là `0x95` và `0x87`).

---

## PHẦN 2: HƯỚNG DẪN SỬ DỤNG VÀ VÍ DỤ FATFS

Vì lớp `SdCard` chỉ đóng vai trò giao tiếp (driver) ở tầng vật lý, nên khi lập trình ứng dụng, sẽ thao tác chủ yếu thông qua bộ thư viện quản lý hệ thống tệp tin **FatFs** (các hàm bắt đầu bằng chữ `f_`).

### 2.1 Bật FatFs trong STM32CubeMX
Để có bộ thư viện FatFs và file `user_diskio.c`, cần kích hoạt nó trong phần mềm STM32CubeMX.
1. Mở file `.ioc`, tìm đến mục **Middleware and Software Packs** -> **FATFS**.
2. Đánh dấu tick chọn **User-defined** (Không chọn SD Card vì chuẩn đó dùng giao tiếp SDIO/SDMMC cứng, thư viện của chúng ta dùng SPI).
3. Trong bảng cấu hình bên dưới, chỉnh thông số quan trọng sau:
   - `MAX_SS` (Maximum Sector Size): Bắt buộc phải là **512**.
   - `USE_LFN` (Use Long Filename): Nếu muốn đọc nhạc có tên dài như `BaiHatCuaToi.mp3`, hãy chọn **Enabled with static working buffer** hoặc **dynamic working buffer**.
4. Bấm **Generate Code** để CubeMX tự sinh ra thư viện FatFs.

### 2.2 Cấu hình liên kết FatFs (user_diskio.c)
Trước khi gọi được bất kỳ hàm nào ở ứng dụng, bắt buộc phải chỉ cho FatFs biết cần gọi các hàm tầng vật lý SPI ở đâu.

Mở file `FATFS/Target/user_diskio.c` (được sinh tự động bởi STM32CubeMX) và `#include "SdCard.h"`, sau đó điền các hàm C-Wrapper vào các khối `USER CODE BEGIN`:

```c
/* USER CODE BEGIN DECL */
#include "SdCard.h"
/* USER CODE END DECL */

DSTATUS USER_initialize (BYTE pdrv) {
  /* USER CODE BEGIN INIT */
    return sdCard_initialize(); 
  /* USER CODE END INIT */
}

DSTATUS USER_status (BYTE pdrv) {
  /* USER CODE BEGIN STATUS */
    return sdCard_status(); 
  /* USER CODE END STATUS */
}

DRESULT USER_read (BYTE pdrv, BYTE *buff, DWORD sector, UINT count) {
  /* USER CODE BEGIN READ */
    return (DRESULT)sdCard_read(buff, sector, count);
  /* USER CODE END READ */
}

DRESULT USER_write (BYTE pdrv, const BYTE *buff, DWORD sector, UINT count) {
  /* USER CODE BEGIN WRITE */
    return (DRESULT)sdCard_write(buff, sector, count);
  /* USER CODE END WRITE */
}

DRESULT USER_ioctl (BYTE pdrv, BYTE cmd, void *buff) {
  /* USER CODE BEGIN IOCTL */
    return (DRESULT)sdCard_ioctl(cmd, buff);
  /* USER CODE END IOCTL */
}
```

### 2.3 Khai báo và Nhận diện thẻ (Mount)
Quy trình nhận thẻ nhớ yêu cầu cung cấp đường truyền SPI, khởi tạo các chân điều khiển phụ, và ra lệnh cho FatFs đọc bảng phân vùng FAT32.

> [!NOTE]
> **Yêu cầu bắt buộc:** Thư viện `SdCard` được xây dựng dựa trên lõi giao tiếp `SpiBus` của dự án. Vì vậy, trong file `main.cpp` (hoặc file gọi), bắt buộc phải `#include "SpiBus.h"` và `#include "SdCard.h"` cùng với thư viện của FatFs (`#include "ff.h"`).

```cpp
// 1. Khai báo SPI Bus (Quản lý phần cứng truyền nhận - DMA hoặc Polling)
SpiBus spi2_bus(SPI2); 

// 2. Khai báo Đối tượng Thẻ Nhớ (Gắn SPI Bus vào)
// Cấu hình ví dụ: Chân CS=PB12, Chân báo thẻ CD=PB14, Chân nguồn PWR=PB15
SdCard sd_card(&spi2_bus, GPIOB, GPIO_PIN_12, GPIOB, GPIO_PIN_14, GPIOB, GPIO_PIN_15);

// 3. Biến toàn cục chứa cấu trúc ổ đĩa FatFs
FATFS fs;

void setup() {
    sd_card.init(); // Thiết lập các chân GPIO an toàn

    // 4. "Mount" thẻ nhớ để FatFs nhận diện hệ thống file FAT32
    // Tham số cuối = 1: Ép vi điều khiển quét thẻ ngay lập tức để phát hiện lỗi
    FRESULT fres = f_mount(&fs, "", 1);
    
    if (fres == FR_OK) {
        // ✅ Nhận thẻ thành công!
    } else {
        // ❌ Lỗi: Thẻ hỏng, định dạng sai (exFAT, NTFS), hoặc chưa cắm
    }
}
```

### 2.4 Đọc File Văn Bản Cơ Bản
Để đọc một đoạn văn bản nhỏ từ thẻ nhớ, ví dụ đọc file cấu hình máy.

```cpp
FIL myFile;
char readBuffer[100];
UINT bytesRead;

// Mở file TEST.TXT với cờ FA_READ (Chỉ để đọc)
if (f_open(&myFile, "TEST.TXT", FA_READ) == FR_OK) {
    // Đọc tối đa 99 byte từ file vào mảng readBuffer
    f_read(&myFile, readBuffer, sizeof(readBuffer)-1, &bytesRead);
    
    // Đóng đuôi chuỗi C-string để có thể printf hoặc hiển thị lên LCD
    readBuffer[bytesRead] = '\0'; 
    
    // LUÔN LUÔN phải đóng file sau khi xử lý xong để giải phóng bộ nhớ!
    f_close(&myFile); 
}
```

### 2.5 Ghi File Log Hệ Thống
Để ghi đè hoặc ghi tiếp dữ liệu vào một file trên thẻ.

```cpp
FIL myFile;
UINT bytesWritten;
const char* data = "Hello VNATECH!\r\n";

// FA_WRITE: Chế độ cho phép Ghi
// FA_CREATE_ALWAYS: Mở file, nếu file đã có thì xóa sạch và tạo file mới tinh.
// FA_OPEN_APPEND: Mở file, con trỏ chuột nhảy tới dòng cuối cùng (Dùng ghi Log).
if (f_open(&myFile, "LOG.TXT", FA_WRITE | FA_OPEN_APPEND) == FR_OK) {
    
    // Đẩy chuỗi data vào file
    f_write(&myFile, data, strlen(data), &bytesWritten);
    
    // Chốt dữ liệu. Nếu mất điện trước khi f_close, file sẽ trống rỗng!
    f_close(&myFile); 
}
```

---

## PHẦN 3: BÍ QUYẾT TỐI ƯU TỐC ĐỘ THẺ NHỚ

Ở chế độ SPI 18MHz, tốc độ lý thuyết là 2.25 MB/s (truyền liên tục 100%). Khi chạy thực tế trong dự án này (đã sử dụng phần cứng DMA đồng bộ), tốc độ đọc đo được đạt khoảng **~1.5 MB/s** (tương đương 68% lý thuyết). Sự hao hụt này là do vi điều khiển luôn phải tiêu tốn thêm thời gian trễ giữa các khối (block) để xử lý ngắt và truyền thêm các gói tín hiệu cấu trúc (như lệnh giao tiếp, cờ hiệu, CRC). Tốc độ này đã rất dư dả để phát nhạc MP3.
Với tác vụ ghi, tốc độ chỉ đạt khoảng **~0.4 MB/s** vì bị rào cản vật lý: mỗi khi truyền xong 1 khối dữ liệu, chip nhớ Flash bên trong thẻ cần thời gian để lưu mảng dữ liệu đó vào ô nhớ. Trong lúc chờ Flash ghi xong này, vi điều khiển bắt buộc phải đợi đường dây rảnh mới được gửi tiếp khối khác.

Nếu làm các dự án cần đọc dữ liệu lớn hoặc ghi Log tốc độ cao, bắt buộc phải nhớ các quy tắc sau:

1. **Luôn dùng mảng đệm (Buffer) lớn:** 
   Thẻ nhớ (Flash NAND) có cấu trúc vật lý xử lý theo từng khối **512 bytes**. Đừng bao giờ gọi lệnh `f_write` hay `f_read` lắt nhắt 1 byte hoặc 2 byte một lần, nó sẽ khiến tốc độ chậm đi hàng chục lần vì Overhead tạo lệnh SPI. Hãy tạo mảng buffer lớn (ví dụ `uint8_t buffer[4096];` - tương đương 8 khối) và đọc/ghi hẳn 4KB một lần.

2. **Hạn chế Open/Close liên tục:** 
   Lệnh `f_open` và `f_close` rất nặng nề vì nó phải ra lệnh cho vi điều khiển sửa đổi bảng định vị FAT của ổ đĩa (FAT Table). 
   - Nếu cần lưu file Log liên tục mỗi giây (VD: Máy đo nhiệt độ), hãy mở file (`f_open`) **1 lần duy nhất** lúc bật máy. 
   - Mỗi khi ghi xong 1 dòng dữ liệu, hãy dùng lệnh `f_sync(&myFile)` để ép thẻ lưu dữ liệu từ RAM xuống Flash an toàn mà không bị mất nếu cúp điện. 
   - Chỉ gọi `f_close` khi hệ thống chuẩn bị tắt nguồn.

3. **Format định dạng FAT32 chuẩn:** 
   Thư viện FatFs của chúng ta chỉ làm việc mượt mà nhất với FAT32. Hãy cắm thẻ vào máy tính, Format theo chuẩn **FAT32** (tuyệt đối không dùng exFAT hay NTFS). Ngoài ra, chọn thông số **Allocation unit size** (kích thước Cluster) lớn nhất có thể (VD: 32KB hoặc 64KB) để giảm độ phân mảnh, giúp hệ thống không phải nhảy sector nhiều khi quét file.
