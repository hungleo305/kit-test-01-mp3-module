# Kit Test 01 - Kết nối MP3


## 1. Mô tả dự án
Dự án **Kết nối MP3** được phát triển nhằm mục đích tích hợp đa dạng các module phần cứng cơ bản (MP3, Thẻ nhớ, RFID, Cảm biến, LED 7 đoạn, Relay, CAN Bus) thành một hệ thống lớn, đồng nhất. 


---

## 2. Phần cứng và Kết nối (Pinout)

Hệ thống bao gồm các thành phần sau và được nối với vi điều khiển STM32F103:

| Linh kiện | Phân loại | Chân kết nối (Pin) | Ghi chú |
| :--- | :--- | :--- | :--- |
| **Cảm biến hồng ngoại** | Đầu vào | `PC13` | Đếm sản phẩm. Đã hàn cứng trên mạch. |
| **Relay** | Đầu ra | `PC7` | Dùng chung chân **Clock** với LED 7 đoạn. |
| **Còi chíp (Buzzer)** | Đầu ra | `PC15` | Báo hiệu khởi động, quẹt thẻ và cảm biến. |
| **LED Main (Onboard)**| Đầu ra | `PB2` | Luôn nháy chu kỳ 100ms ON / 100ms OFF. |
| **LED 7 Đoạn (74HC595)** | Hiển thị | Data:`PC5`, Clock:`PC7`, Latch:`PC6` | Được xử lý chống nhiễu phần mềm. |
| **Mạch thời gian thực (RTC)**| Giao tiếp | SCL: `PB6`, SDA: `PB7` | Giao tiếp qua I2C1. |
| **Khe cắm thẻ nhớ Micro SD**| Giao tiếp | `SPI1` (CS: `PC8`, Pwr: `PB1`, Detect: `PB0`) | Chân ngầm định: PA5, PA6, PA7. Định dạng FAT32. |
| **Đầu đọc thẻ RFID (RC522)**| Giao tiếp | `SPI1` (CS:`PC14`, RST:`PA15`) | Chân tự động Remap: PB3, PB4, PB5. |
| **Module nhạc MP3 (VS1011E)**| Giao tiếp | `SPI2` (CS:`PC3`, DCS:`PB12`, DREQ:`PA1`, RST:`PA0`, Amp:`PA4`, Pwr:`PC2`) | Chân ngầm định: PB13, PB14, PB15. Giải mã âm thanh. |
| **Mạng CAN Bus** | Giao tiếp | RX: `PA11`, TX: `PA12` | Giao tiếp 2 chiều với PC. |

---

## 3. Hướng dẫn sử dụng Mạch (Dành cho Người dùng)

### 3.1. Quy trình Khởi động
- **Cấp nguồn 9 VDC** vào mạch. Hãy kiểm tra kỹ các rắc cắm trước khi cấp điện để tránh cháy nổ.
- Khi có điện, còi chip sẽ kêu **2 tiếng "bíp" ngắn** (50ms ON, 100ms OFF) để báo hiệu hệ thống sẵn sàng.
- Đèn LED Main trên mạch sẽ bắt đầu nháy liên tục (100ms ON / 100ms OFF) làm Heart-beat báo hiệu chương trình không bị treo.

### 3.2. Hiển thị & Đếm sản phẩm
- Màn hình LED 7 đoạn sẽ tự động luân phiên hiển thị: **5 giây hiển thị Số lượng đếm** -> **5 giây hiển thị Thời gian thực (hh:mm)**.
- Khi có vật cản che qua cảm biến hồng ngoại: Còi kêu 1 tiếng, bộ đếm tăng thêm 1. Nếu đạt "Mục tiêu" (Target cài đặt qua CAN), loa sẽ phát câu thông báo "Đã đạt mục tiêu".

### 3.3. Tính năng Thẻ RFID (Lưu lệnh trực tiếp vào thẻ)
Hệ thống không lưu UID trong chip, mà lưu thẳng mã lệnh (chuỗi ký tự) vào ô nhớ số 4 của chiếc thẻ RFID. Có 3 loại thẻ chức năng:
1.  **Thẻ 01 (Reset):** Khi quẹt -> Còi kêu 1 tiếng -> Loa phát âm thanh "Reset giá trị về 0" -> Reset bộ đếm trên màn hình. (Nội dung thẻ: `RESET_DEM`)
2.  **Thẻ 02 (Bật Relay):** Khi quẹt -> Còi kêu 1 tiếng -> Loa phát âm thanh "Bật relay" -> Relay kích hoạt. (Nội dung thẻ: `BAT_RELAY`)
3.  **Thẻ 03 (Đảo Relay):** Khi quẹt -> Còi kêu 1 tiếng -> Loa phát âm thanh "Đảo trạng thái relay" -> Relay đổi trạng thái. (Nội dung thẻ: `DAO_RELAY`)

### 3.4. Quản lý Thẻ RFID & Đổi Mật Khẩu qua CAN Bus
Hệ thống cho phép nạp tính năng, xóa thẻ và đổi mật khẩu (Key A) của thẻ RFID thông qua mạng CAN Bus (Gửi từ máy tính xuống):
- **Ghi tính năng mới (CAN ID `0x204`):** Gửi Byte 0 = `0x01` (Lệnh Reset), `0x02` (Lệnh Bật Relay), `0x03` (Lệnh Đảo Relay). Sau đó quẹt thẻ trắng vào máy để nạp.
- **Xóa thẻ (CAN ID `0x205`):** Gửi Byte 0 = `0x00`. Sau đó quẹt thẻ vào để xóa.
- **Đổi mật khẩu thẻ (CAN ID `0x205`):** Gửi Byte 0 = `0x01`, các Byte từ 1 đến 6 là **Mật khẩu mới**. Quẹt thẻ vào hệ thống sẽ ghi đè mật khẩu mới (Key A) vào Sector Trailer (Block 7).
- **Cấp mật khẩu tạm để xác thực (CAN ID `0x206`):** Nếu thẻ đã bị đổi mật khẩu, gửi 6 Byte mật khẩu cũ qua ID `0x206`. Quẹt thẻ vào, hệ thống sẽ dùng mật khẩu tạm này để xác thực thay cho key mặc định.

Mỗi khi quẹt thẻ, dữ liệu đọc được từ thẻ sẽ được đóng gói và bắn qua mạng CAN Bus về máy tính.

### 3.5. Cấu hình qua mạng CAN Bus
Hệ thống cho phép cấu hình từ xa thông qua PC/Máy tính bằng CAN Bus:
- **ID `0x201`**: Cài đặt Mục tiêu đếm (Mã BCD). Còi báo tít tít nếu nhận thành công.
- **ID `0x202`**: Ghi đè bộ đếm hiện tại (Mã BCD).
- **ID `0x203`**: Cài đặt Thời gian thực cho mạch RTC (Mã BCD - hh:mm:ss).
- **Dữ liệu mạch gửi lên:** Giá trị đếm hiện tại, Giá trị mục tiêu (Target), Thời gian thực (RTC), Dữ liệu thẻ RFID vừa quẹt.
- **Dữ liệu máy tính nạp xuống:** Cài đặt lại Giá trị đếm, Giá trị mục tiêu, Chỉnh lại giờ RTC, và **Ghi đè tính năng mới vào thẻ RFID** (chuyển thẻ thường thành thẻ Reset/Bật/Đảo).

---

## 4. Hướng dẫn sử dụng Thư viện (Dành cho Lập trình viên)

Dự án này sử dụng kiến trúc Hướng đối tượng (OOP C++) kết hợp thao tác thanh ghi (Bare-metal) để đạt hiệu năng tối đa, loại bỏ sự rề rà của HAL và dễ dàng tái sử dụng. Dưới đây là hướng dẫn chi tiết cho từng class:

### 4.1. Thư viện SPI Động (`SdCard.h` & `Rfid.h`)
Hai thiết bị Thẻ nhớ và RFID cùng dùng chung một bộ phần cứng `SPI1` nhưng cắm ở 2 cụm chân khác nhau. Thư viện tự động xử lý đổi chân (Remap) mỗi khi thiết bị cần giao tiếp.
- **Khai báo:**
```cpp
// RFID tự động cấu hình Remap (PB3, PB4, PB5) khi gọi hàm
Rfid rfid(SPI1, GPIOC, 1<<14, GPIOA, 1<<15);
// SdCard tự động dùng chân gốc (PA5, PA6, PA7)
SdCard sdCard(SPI1, GPIOC, 1<<8);
```
Bên trong thư viện, khi gửi dữ liệu, thanh ghi `AFIO->MAPR` sẽ tự động đóng/mở công tắc để "lái" tín hiệu SPI đi đúng đường, giúp lập trình viên không cần bận tâm về xung đột phần cứng.

### 4.2. Thư viện Audio MP3 (`Mp3.h`)
Thư viện điều khiển chip VS1011E theo cơ chế **Đa nhiệm không đồng bộ (Non-blocking)**.
- **Khởi tạo:** Nạp đúng các chân điều khiển, lưu ý chân cấp nguồn (PWR_EN) là `PC2`.
```cpp
Mp3 mp3(&hspi2, GPIOA, 1<<0, GPIOA, 1<<1, GPIOB, 1<<12, GPIOC, 1<<3);
mp3.init();
```
- **Phát nhạc:** Hàm `mp3.play("TENFILE.MP3")` làm nhiệm vụ mở file thẻ nhớ, nếu có lỗi trả về `false`. 
- **Cơ chế Bơm dữ liệu (Quan trọng):** Ở vòng lặp `while(1)`, BẮT BUỘC phải gọi liên tục hàm `mp3.process()`. Hàm này sẽ kiểm tra chân `DREQ` của MP3, hễ chip MP3 rảnh thì MCU sẽ tự trích xuất 32 byte từ SD Card đẩy vào MP3. Cơ chế này giúp MP3 luôn hát mượt mà không làm treo hệ thống.

### 4.3. Thư viện Màn hình LED chống nhiễu (`Led7Segment.h`)
Do mạch phần cứng thiết kế chân `PC7` vừa làm Data của LED, vừa làm chân kích Relay. Để LED không nháy loạn khi bật/tắt Relay, thư viện sử dụng kỹ thuật Tiêm Callback (Dependency Injection).
```cpp
// Hàm ẩn danh lấy trạng thái Relay
bool getRelayStatus() { return relay.isOn(); }
// Ném vào tham số thứ 7 của hàm khởi tạo
Led7Segment dp(GPIOC, 1<<5, GPIOC, 1<<7, GPIOC, 1<<6, getRelayStatus);
```
Khi chốt dữ liệu bằng chân Latch, thư viện tự động kiểm tra lại trạng thái chân `PC7` (để trả về đúng mức điện áp của Relay), triệt tiêu hoàn toàn sự cố chớp nháy.

### 4.4. Thư viện Giao tiếp CAN Bus (`CanBus.h`)
Được thiết kế để tối giản hóa sự phức tạp của thư viện HAL. Hỗ trợ Filter tự động để nhận đúng luồng dữ liệu.
- **Gửi Frame:** 
```cpp
uint8_t data[4] = {0x12, 0x34, 0x56, 0x78};
myCan.sendFrame(0x102, data, 4); // ID 0x102, dài 4 byte
```
- **Nhận Frame:** Trong hàm ngắt `HAL_CAN_RxFifo0MsgPendingCallback`, gọi `myCan.readFrame(&rxId, rxData, &rxLen)` để bóc tách trực tiếp mã ID và Array Dữ liệu cực kỳ đơn giản.

### 4.5. Thư viện Thời gian thực RTC (`Rtc.h`)
Giao tiếp I2C cứng (viết bằng thanh ghi) để xử lý chip DS1307. Tốc độ đọc ghi cực nhanh không bị kẹt I2C Timeout của HAL.
- Lấy và nạp giờ: `rtc.getTime(h, m, s);` và `rtc.setTime(h, m, s);`
- Hỗ trợ cảnh báo Pin CMOS yếu: `rtc.isTimeLost()` sẽ trả về `true` nếu mạch mới cấp điện và phát hiện mốc thời gian bị trôi về năm mặc định.

### 4.6. Lõi Đa nhiệm bằng `SystemTimer`
Để kiến trúc Non-blocking chạy trơn tru, dự án **nghiêm cấm sử dụng `delay_ms()`** hay `HAL_Delay()` trong vòng lặp chính. Thay vào đó, dùng `SystemTimer::getTick()` để lập lịch.
```cpp
uint32_t currentTick = SystemTimer::getTick();
if (currentTick - lastActionTick >= 2000) { 
    lastActionTick = currentTick;
    // Việc này sẽ được thực hiện đúng chu kỳ 2 giây
}
```

---

## 5. Hướng dẫn Tích hợp & Cài đặt Môi trường (Git & IDE)

### 5.1. Yêu cầu phần mềm
- **IDE:** IAR Embedded Workbench for ARM 8.40.1.
- **Khởi tạo Code:** STM32CubeMX 6.17.0 (Chỉ dùng để mồi Clock Tree và bộ khung sườn ban đầu, phần code logic tự viết bằng thanh ghi).

