# Hướng Dẫn Sử Dụng Và Tái Sử Dụng Dự Án Kit Test 01

Bản hướng dẫn này được biên soạn với ngôn ngữ đơn giản, rõ ràng, giúp người mới tiếp cận có thể dễ dàng sử dụng mạch cũng như tái sử dụng mã nguồn (code) cho các dự án khác.

---

## 1. Chức năng của mạch
Dự án sử dụng vi điều khiển STM32F103 để điều khiển các module phần cứng thực hiện các tác vụ sau:
- **Cảm biến hồng ngoại:** Phát hiện vật cản và đếm số lượng vật thể đi qua.
- **Màn hình LED 7 đoạn:** Hiển thị số lượng vừa đếm được và hiển thị thời gian thực (Giờ:Phút).
- **Đầu đọc thẻ từ RFID:** Quét thẻ để thực hiện các lệnh như cài đặt lại bộ đếm về 0, bật hoặc tắt công tắc (Relay).
- **Module MP3 & Thẻ nhớ SD:** Phát các câu thông báo bằng âm thanh khi người dùng quẹt thẻ hoặc khi số đếm đạt mục tiêu. Có khả năng ghi nhớ và tự động phát tiếp bài nhạc dang dở nếu thẻ nhớ bị rút ra rồi cắm lại.
- **Mạng CAN Bus:** Kết nối vi điều khiển với máy tính để theo dõi dữ liệu hiển thị và thay đổi cấu hình từ xa.

---

## 2. Kết nối phần cứng (Pinout)
Hệ thống bao gồm các thành phần sau và được nối với vi điều khiển STM32F103RCT6:

| Thiết bị | Chân kết nối (Pin) | Chức năng chi tiết |
| :--- | :--- | :--- |
| **Cảm biến hồng ngoại** | `PC13` | Nhận tín hiệu khi có vật cản. Đã hàn cứng trên mạch. |
| **Còi báo (Buzzer)** | `PC15` | Phát âm thanh bíp báo hiệu khởi động, quẹt thẻ hoặc khi đếm. |
| **Công tắc (Relay)** | `PC7` | Đóng/ngắt điện ngoài. Dùng chung chân Clock với màn hình LED 7 đoạn. |
| **Đèn LED Main (Onboard)** | `PB2` | Nháy chớp/tắt chu kỳ 100ms báo hiệu vi điều khiển vẫn đang chạy. |
| **Màn hình LED 7 Đoạn** | Data: `PC5`, Clock: `PC7`, Latch: `PC6` | Giao tiếp với IC CD4094 để hiển thị số đếm và giờ. |
| **Mạch thời gian thực (RTC)** | SCL: `PB6`, SDA: `PB7` | Giao tiếp I2C1 với chip đồng hồ DS1307. |
| **Mạng CAN Bus** | RX: `PA11`, TX: `PA12` | Giao tiếp 2 chiều với máy tính (Cangaroo). |
| **Khe cắm thẻ nhớ SD** | **SPI1:** SCK: `PA5`, MISO: `PA6`, MOSI: `PA7`, CS: `PC8`, Pwr (Nguồn): `PB1`, Detect (Báo thẻ): `PB0` | Giao tiếp thẻ MicroSD (Định dạng FAT32). Cấp nguồn có điều khiển để cho phép cắm/rút nóng thẻ. |
| **Đầu đọc thẻ RFID (RC522)**| **SPI1 Remap:** SCK: `PB3`, MISO: `PB4`, MOSI: `PB5`, CS: `PC14`, RST: `PA15` | Dùng chung phần cứng SPI1 với thẻ nhớ nhưng tự động chuyển đổi sang nhánh chân PB3, PB4, PB5. |
| **Module MP3 (VS1011E)** | **SPI2:** SCK: `PB13`, MISO: `PB14`, MOSI: `PB15`, CS (Lệnh): `PC3`, DCS (Dữ liệu): `PB12`, DREQ (Trạng thái): `PA1`, RST: `PA0`, Amp: `PA4`, Pwr (Nguồn): `PC2` | Nhận file âm thanh từ thẻ nhớ rồi giải mã. Dùng bộ truyền dữ liệu SPI2 độc lập để phát nhạc liên tục không bị gián đoạn. |

---

## 3. Hướng dẫn sử dụng mạch (Dành cho người dùng)

Mạch được thiết kế để giao tiếp với người dùng thông qua âm thanh (Còi Buzzer, Loa MP3) và hình ảnh (LED 7 đoạn, LED báo hiệu). Dưới đây là ý nghĩa toàn bộ các tín hiệu của hệ thống:

### 3.1. Các tín hiệu Còi báo (Buzzer)
Còi báo là công cụ giao tiếp chính của mạch, tùy theo số tiếng kêu và độ dài ngắn mà báo hiệu các trạng thái khác nhau:
- **Kêu 1 tiếng Bíp ngắn (50ms - 150ms):**
  - Khi có vật cản che qua cảm biến.
  - Khi đọc thành công mã thẻ RFID.
  - Khi máy tính (CAN Bus) gửi lệnh cấu hình thành công (cài giờ, cài mục tiêu, ghi đè bộ đếm).
- **Kêu 1 tiếng Bíp dài (500ms):**
  - Khi Ghi/Nạp tính năng mới vào thẻ RFID thành công.
- **Kêu 2 tiếng Bíp ngắn (100ms):**
  - Báo lỗi: Quẹt thẻ không đúng định dạng lệnh, sai mật khẩu thẻ, hoặc máy tính gửi sai dữ liệu cấu hình.
  - Cảnh báo: Quẹt thẻ lệnh "Bật Relay" nhưng Relay đang bật sẵn, hoặc thẻ "Reset" nhưng bộ đếm đang là 0.
- **Kêu 2 tiếng Bíp cực nhanh (50ms):**
  - Báo hiệu hệ thống vừa nhận lệnh từ máy tính và đang ở chế độ "Chờ nạp thẻ". Lúc này bạn cần quẹt thẻ vào để nạp/xóa/đổi key.
- **Kêu 5 tiếng Bíp liên tục (100ms):**
  - Báo lỗi Module MP3: Không tìm thấy file âm thanh trong thẻ nhớ SD (ví dụ thiếu file `DAT.MP3`, `BAT.MP3`...).
- **Kêu 5 tiếng Bíp cực nhanh (50ms):**
  - Báo lỗi hệ thống thẻ: Không đọc được dữ liệu thẻ từ (block 0) hoặc thẻ bị hỏng.

### 3.2. Hiển thị LED 7 Đoạn & LED Onboard
- **Đèn LED Main (Xanh lá):** Luôn chớp tắt cực nhanh (chu kỳ 100ms sáng / 100ms tắt) để báo hiệu "nhịp tim" (Heartbeat), chứng tỏ vi điều khiển không bị treo.
- **Màn hình LED 7 đoạn (Màu đỏ):** 
  - **Chế độ luân phiên:** Tự động chuyển đổi: 5 giây hiển thị "Số lượng sản phẩm" -> 5 giây hiển thị "Thời gian thực".
  - **Hiển thị giờ:** Khi đang hiện giờ, dấu chấm ở giữa hai số (Colon) sẽ nhấp nháy chớp/tắt mỗi nửa giây.
  - **Hiển thị số đếm:** Dấu chấm ở giữa sẽ bị tắt đi.
  - **Chế độ Ưu tiên:** Đang ở chế độ xem giờ mà có hàng hóa đi qua cảm biến, màn hình sẽ lập tức quay về chế độ hiển thị Số đếm.
  - **Chống nhiễu Relay:** Khi đóng/ngắt điện 220V qua Relay, màn hình LED sẽ được tắt chớp nhoáng 1 tích tắc để tránh nhiễu do chung đường truyền.

### 3.3. Các tính năng Vận hành tự động
- **Đếm và báo Còi/Loa:** 
  Khi có vật cản che qua cảm biến, còi kêu 1 tiếng và số đếm trên màn hình tăng lên 1. Nếu số đếm đạt mức "Mục tiêu", loa MP3 sẽ phát câu thông báo "Đã đạt mục tiêu" (File `DAT.MP3`).
- **Sử dụng thẻ từ RFID:** 
  Hệ thống không quản lý ID thẻ (UID) mà đọc trực tiếp chuỗi chữ cái (Text) lưu bên trong ô nhớ của thẻ. Bạn quẹt thẻ, hệ thống sẽ thực hiện lệnh tương ứng:
   - **Thẻ Lệnh Reset:** Đọc thẻ -> Loa phát âm thanh "Reset giá trị về 0" (`RESET.MP3`) -> Số đếm trở về 0.
   - **Thẻ Lệnh Bật Relay:** Đọc thẻ -> Loa phát âm báo (`BAT.MP3`) -> Relay đóng lại.
   - **Thẻ Lệnh Đảo Relay:** Đọc thẻ -> Loa phát âm báo (`DAO.MP3`) -> Relay đổi trạng thái (Tắt thành Bật, Bật thành Tắt).
   - **Thẻ Lệnh Phát Nhạc:** Đọc thẻ -> Phát bài nhạc giải trí (`MUSIC.MP3`).
- **Rút cắm thẻ nhớ an toàn:** 
  Hệ thống hỗ trợ cắm rút thẻ nhớ nóng (Hot-plug). Nếu rút thẻ ra khi đang phát nhạc, mạch sẽ văng lỗi nhưng không bị treo. Cắm lại thẻ, mạch sẽ khôi phục.

---

## 4. Hướng dẫn sử dụng thư viện (Dành cho lập trình viên)

Dự án được chia thành các thư viện độc lập mang tính đóng gói cao (C++ OOP). Lập trình viên có thể copy các cặp file `.h` và `.cpp` để tái sử dụng ngay cho dự án khác mà không cần sửa đổi nhiều.

### Nguyên tắc cốt lõi: Không sử dụng lệnh Delay
Toàn bộ mã nguồn nghiêm cấm sử dụng lệnh `HAL_Delay()`. Thay vào đó, sử dụng hàm `SystemTimer::getTick()` để lấy thời gian đếm và so sánh. Cơ chế chạy không đồng bộ (Non-blocking) này giúp chip vi điều khiển không bao giờ bị "treo" để chờ đợi, đảm bảo nhạc phát mượt mà và đầu đọc RFID luôn có thể nhận thẻ bất cứ lúc nào.

### Cách tái sử dụng các thư viện chính:

Để tái sử dụng, bạn chỉ cần copy cặp file `.h` và `.cpp` tương ứng vào dự án của bạn (sử dụng VĐK dòng STM32F1) và `#include` chúng. Dưới đây là cách khai báo và gọi hàm cơ bản:

- **Thư viện âm thanh MP3 (`Mp3.h`):** 
  Phải truyền con trỏ của giao diện SPI và cấu hình các chân điều khiển tương ứng khi khởi tạo. Thay thế `PORT_x` và `PIN_y` bằng cấu hình thực tế trên mạch của bạn:
  ```cpp
  // Khai báo và khởi tạo module MP3
  Mp3 mp3(&hspi_x, PORT_RST, PIN_RST, PORT_DREQ, PIN_DREQ, PORT_DCS, PIN_DCS, PORT_CS, PIN_CS);
  mp3.init();
  
  // Ra lệnh phát nhạc
  mp3.play("ten_file.MP3");
  
  // BẮT BUỘC đặt trong vòng lặp chính (while 1)
  while(1) {
      mp3.process(); // Tự động trích xuất dữ liệu từ SD Card cấp cho MP3
  }
  ```

- **Thư viện LED 7 Đoạn (`Led7Segment.h`):** 
  Truyền vào các chân kết nối với IC CD4094. Nếu mạch của bạn có dùng chung chân LED với Relay, hãy truyền thêm hàm kiểm tra trạng thái Relay. Nếu không, có thể truyền `nullptr` (hoặc cấu trúc lại hàm khởi tạo).
  ```cpp
  // Viết hàm trung gian lấy trạng thái thực tế của Relay (nếu có dùng chung chân)
  bool checkRelay() { return relay.isOn(); }
  
  // Khởi tạo thư viện LED (thay PORT_x, PIN_y bằng chân kết nối thực tế)
  Led7Segment dp(PORT_DATA, PIN_DATA, PORT_CLK, PIN_CLK, PORT_LATCH, PIN_LATCH, checkRelay);
  
  // Hiển thị số lên màn hình
  dp.setValue(9999);
  dp.update(false); // false = tắt dấu chấm giữa
  ```

- **Thư viện quản lý chung SPI (`SdCard.h` & `Rfid.h`):** 
  Truyền vào bộ SPI được sử dụng cùng với các chân CS (Chip Select) và chân RST/NIRQ tương ứng.
  ```cpp
  // Khai báo chung cổng SPI (ví dụ SPI1) nhưng chân CS và ngắt khác nhau
  Rfid rfid(SPI1, PORT_CS_RFID, PIN_CS_RFID, PORT_RST_RFID, PIN_RST_RFID);
  SdCard sdCard(SPI1, PORT_CS_SD, PIN_CS_SD);
  
  // Gọi hàm bình thường, thư viện tự lo việc điều hướng cấu hình bên dưới
  sdCard.mount();
  rfid.readCard(cardData);
  ```

- **Thư viện Đồng hồ (`Rtc.h`):** 
  Được tối ưu xử lý I2C bằng thanh ghi của STM32F1.
  ```cpp
  uint8_t h, m, s;
  rtc.getTime(h, m, s); // Đọc giờ
  rtc.setTime(14, 30, 0); // Chỉnh giờ
  
  // Kiểm tra thời gian
  if (rtc.isTimeLost()) {
      // Cảnh báo người dùng cần cài lại giờ
  }
  ```

- **Thư viện Cảm biến (`Sensor.h`):**
  Đọc tín hiệu cảm biến không bị nhiễu. Có thể kết hợp với ngắt ngoài hoặc dùng hàm bắt sườn tín hiệu.
  ```cpp
  Sensor sensor(PORT_SENSOR, PIN_SENSOR);
  
  // Hàm isTriggered() tự động bắt sườn tín hiệu, chỉ trả về true 1 lần 
  // duy nhất mỗi khi vật cản đi qua (không bị nhảy số liên tục khi bị che)
  if (sensor.isTriggered()) {
      // Tăng biến đếm hoặc phát âm thanh
  }
  ```

- **Thư viện Còi báo, Đèn và Công tắc (`Buzzer.h`, `Led.h`, `Relay.h`):**
  Các thư viện cơ bản điều khiển đầu ra (GPIO Output). Cung cấp hàm thao tác trực tiếp thay vì phải nhớ cấu trúc GPIO của HAL.
  ```cpp
  Led mainLed(PORT_LED, PIN_LED);
  Buzzer buzzer(PORT_BUZZER, PIN_BUZZER);
  Relay relay(PORT_RELAY, PIN_RELAY);

  mainLed.toggle(); // Đảo trạng thái chớp/tắt
  buzzer.on();      // Bật còi
  relay.off();      // Tắt công tắc
  ```

- **Thư viện Giao tiếp CAN Bus (`CanBus.h`):**
  Thư viện đóng gói các thao tác thiết lập Filter và gửi/nhận rườm rà của HAL thành các hàm rất gọn gàng.
  ```cpp
  // Khởi tạo thư viện, truyền vào con trỏ CAN tương ứng
  CanBus myCan(&hcan);
  myCan.init();
  
  // Gửi 4 byte dữ liệu lên máy tính với ID là 0x102
  uint8_t txData[4] = {0x11, 0x22, 0x33, 0x44};
  myCan.sendFrame(0x102, txData, 4); 
  
  // Nhận dữ liệu (thường đặt trong hàm ngắt HAL_CAN_RxFifo0MsgPendingCallback)
  uint32_t rxId;
  uint8_t rxData[8];
  uint32_t rxLen;
  if (myCan.readFrame(&rxId, rxData, &rxLen)) {
      // Có dữ liệu mới, bắt đầu xử lý theo mã rxId
  }
  ```

- **Bộ định thời hệ thống (`SystemTimer.h`):**
  Đây là "linh hồn" của toàn bộ cơ chế chạy không đồng bộ (Non-blocking) thay thế cho hàm Delay.
  ```cpp
  uint32_t currentTick = SystemTimer::getTick(); // Lấy thời gian (mili-giây)
  
  // Mẫu code thực thực thi một tác vụ lặp lại mỗi 100ms mà không dừng luồng chính
  if (currentTick - lastTick >= 100) {
      lastTick = currentTick;
      mainLed.toggle(); 
  }
  ```
