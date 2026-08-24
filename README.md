# TÀI LIỆU HƯỚNG DẪN SỬ DỤNG VÀ PHÁT TRIỂN FIRMWARE - KIT VNA

Dự án mạch Kit điều khiển đa năng giao tiếp CAN Bus, tích hợp đọc thẻ RFID, thẻ nhớ SD (phát nhạc MP3), cảm biến đếm sản phẩm, và LED 7 đoạn.

---

## 1. CẤU TRÚC THƯ MỤC DỰ ÁN

```Text
kit-test-01-mp3-module/
├── Core/
│   ├── Inc/                # Header thư viện (.h)
│   │   ├── Buzzer.h        # Còi chip (Hỗ trợ hàng đợi Non-blocking)
│   │   ├── CanBus.h        # Mạng CAN (Hỗ trợ Auto-Recovery)
│   │   ├── Led7Segment.h   # LED 7 đoạn
│   │   ├── Mp3.h           # Phát nhạc VS1101E
│   │   ├── Relay.h         # Công tắc
│   │   ├── Rfid.h          # Thẻ RC522
│   │   ├── Rtc.h           # Thời gian DS1307
│   │   ├── SdCard.h        # Thẻ nhớ
│   │   ├── Sensor.h        # Cảm biến đếm
│   │   └── SystemTimer.h   # Bộ đếm Non-blocking
│   └── Src/                # Source code (.cpp)
│       └── main.cpp        # Vòng lặp chính
├── Drivers/                # Thư viện HAL của STM32
├── FATFS/                  # File System dùng để đọc thẻ SD
├── kit_vna_can.dbc         # Database ánh xạ dữ liệu CAN (Cangaroo)
└── README.md               # Tài liệu này
```
---

## 2. HƯỚNG DẪN SỬ DỤNG DÀNH CHO NGƯỜI DÙNG

Phần này hướng dẫn chi tiết cách vận hành mạch từ lúc cắm điện đến lúc sử dụng các tính năng. Bạn chỉ cần làm theo các bước dưới đây.

### 2.1. Cắm điện và Khởi động
- **Cắm điện:** Ngay khi vừa cấp nguồn, mạch sẽ khởi động hệ thống và kêu **2 tiếng Bíp** để báo hiệu đã có điện và sẵn sàng.
- **Đèn LED Onboard :** Đèn LED nhỏ trên thân mạch sẽ chớp nháy liên tục rất nhanh (chu kỳ 0.1 giây sáng / 0.1 giây tắt - tức là chớp 5 lần/giây). Đèn này nháy đều đặn chứng tỏ  vi xử lý đang chạy cực kỳ trơn tru, vòng lặp hoạt động không bị treo .
- **Màn hình LED 7 đoạn:** Sẽ bắt đầu tự động luân phiên hiển thị giữa 2 thông tin (mỗi thông tin hiển thị 5 giây):
  1. **Số lượng sản phẩm đã đếm được:** Dấu chấm ở giữa màn hình sẽ **TẮT**.
  2. **Giờ hiện tại (Giờ:Phút):** Dấu chấm ở giữa màn hình sẽ **NHẤP NHÁY** theo từng giây.

### 2.2. Tín hiệu Còi (Buzzer) và Cảnh báo
Hệ thống còi bíp đã được tinh chỉnh để chạy hoàn toàn đa nhiệm (có hàng chờ thông minh) nên các tiếng bíp sẽ lần lượt phát ra mượt mà, không bao giờ bị dính hoặc nuốt tiếng của nhau. 

Mạch được quy định phản hồi các thao tác thông qua số lượng tiếng bíp như sau:

| Âm thanh | Tình huống | Ý nghĩa chi tiết |
| :--- | :--- | :--- |
| **1 Bíp ngắn** | **Thành công** | - Có sản phẩm chạy qua mắt cảm biến.<br>- Nhận diện thành công có thẻ RFID quẹt qua.<br>- Cập nhật thành công cài đặt mới từ phần mềm máy tính (CAN Bus: Mục tiêu, Bộ đếm, Xem giờ). |
| **2 Bíp ngắn** | **Lỗi Dữ liệu / Sai thao tác** | - **Khởi động:** Báo hiệu mạch vừa khởi động/cấp điện thành công.<br>- **CAN Bus:** Gửi cài đặt Mục tiêu / Số đếm lớn hơn `9999` (Vượt quá giới hạn màn hình LED).<br>- **Quẹt Thẻ (RFID):** Sai mật khẩu thẻ.<br>- **Quẹt Thẻ (RFID):** Lỗi giao tiếp với chip nhớ trong thẻ (không đọc được Block).<br>- **Quẹt Thẻ (RFID):** Dữ liệu trong thẻ bị sai lệch (VD: Không chứa các lệnh hợp lệ như `RESET_DEM`, `BAT_RELAY`...).<br>- **Quẹt Thẻ (RFID) Thao tác thừa:** Ví dụ thẻ mang lệnh bắt "Bật Relay" nhưng Relay đang bật sẵn, hoặc thẻ mang lệnh "Reset bộ đếm" nhưng số đếm đang là 0 sẵn rồi. |
| **3 Bíp ngắn** | **Lỗi Phần cứng / Module MP3** | - **Lỗi Thẻ Nhớ SD:** Thẻ nhớ bị lỗi định dạng (yêu cầu FAT32), chưa cắm thẻ nhớ, hoặc khe cắm hỏng tiếp xúc phần cứng.<br>- **Lỗi Module MP3:** Module âm thanh MP3 bị treo không phản hồi, hoặc thẻ nhớ không có tệp `.MP3` tương ứng với hành động. |
| **1 Bíp Dài**<br>*(Nửa giây)* | **Lỗi Mất Mạng CAN** | - Mạch cảnh báo phần cứng phát hiện đường truyền mạng CAN Bus đang bị đứt cáp hoặc lỗi bộ chuyển đổi máy tính. |

**Tín hiệu Cảnh báo trên Màn hình LED:**
- **Lỗi đứt cáp mạng CAN:** Nếu mạch phát hiện mạng CAN bị đứt, ở chế độ đếm số, **dấu chấm ở giữa màn hình sẽ nhấp nháy liên tục rất nhanh (5 lần/giây)** để cảnh báo dữ liệu không gửi được lên máy tính. Ngay khi khôi phục lại kết nối canbus, phần cứng sẽ tự khôi phục và dấu chấm tự động ngừng nhấp nháy (hệ thống về trạng thái bình thường).
- **Lỗi mất thời gian thực (Hết pin RTC):** Nếu mạch bị mất thời gian thực. Khi hiển thị chế độ Xem giờ, thay vì hiện `Giờ:Phút`, màn hình sẽ hiện chữ `----` nhấp nháy mãi mãi để báo lỗi. Lúc này cần nạp lại thời gian qua CAN Bus (Cangaroo).

### 2.3. Hướng dẫn đếm sản phẩm bằng Cảm biến
- **Cách đếm:** Khi có sản phẩm chạy ngang qua mắt cảm biến, còi kêu 1 tiếng **Bíp** và số trên màn hình tăng lên 1. 
- **Tính năng lấn át:** Nếu màn hình đang ở chế độ "Xem giờ" mà cảm biến bắt được sản phẩm, màn hình sẽ **lập tức nhảy về chế độ Số Đếm** để dễ dàng theo dõi.
- **Đạt mục tiêu:** Khi số lượng sản phẩm đếm được bằng với con số "Mục tiêu" (có thể cài đặt qua máy tính bằng canbus), loa sẽ tự động phát âm thanh "Đã đạt mục tiêu" (tên file là `DAT.MP3`).

### 2.4. Hướng dẫn sử dụng Thẻ Nhớ MP3 (Thẻ SD)
- **Chuẩn bị thẻ:** Rút thẻ ra cắm vào máy tính, Format thẻ với định dạng **FAT32**. Sau đó copy các file âm thanh (đuôi file là `.mp3`) trực tiếp vào thẻ, không bỏ vào thư mục nào cả. 
- **Rút/Cắm thẻ khi đang chạy (Hot-plug):** Nếu đang phát âm thanh mà vô tình rút thẻ ra -> Mạch lập tức tự động ngắt amply để loa không bị nổ lụp bụp, đồng thời tự động lưu lại vị trí bài hát đang nghe dở. Khi cắm thẻ lại -> Mạch sẽ tự động mount thẻ nhớ và **tự động hát tiếp** đoạn âm thanh vừa bị ngắt.

### 2.5. Hướng dẫn Quẹt thẻ Thao tác (RFID)
Mạch đi kèm bộ thẻ từ. Quẹt các thẻ này vào anten (miếng vuông in chữ RC522) để ra lệnh cho mạch làm việc:
- Quẹt thẻ **RESET**: Mạch tự động xóa số đếm sản phẩm về 0 và loa phát bài `RESET.MP3`.
- Quẹt thẻ **BAT**: Bật công tắc điện (Relay) trên mạch, loa phát bài `BAT.MP3`.
- Quẹt thẻ **DAO**: Đổi ngược trạng thái công tắc (Đang bật thành tắt, đang tắt thành bật), loa phát bài `DAO.MP3`.
- Quẹt thẻ **PHAT_NHAC**: Ra lệnh cho loa phát một bài nhạc giải trí tên là `MUSIC.MP3`.
*( Có thể quẹt liên tục nhiều thẻ khác nhau, các âm thanh sẽ tự động xếp hàng và phát lần lượt).*

### 2.6. Hướng dẫn dùng phần mềm máy tính (Cangaroo qua CAN Bus)
Bạn cắm mạch vào máy tính qua bộ chuyển đổi CAN. Mở phần mềm **Cangaroo** trên máy tính:
1. Load file có sẵn tên là `kit_vna_can.dbc` (có sẵn trong thư mục dự án) vào phần mềm.
2. Lúc này toàn bộ các ID kỹ thuật sẽ tự động biến thành tiếng Việt (Ví dụ: *Thoi_Gian_Thuc_RTC, Muc_Tieu...*).
3. **Cách đổi số lượng Mục Tiêu:** Tìm đến khung CAN TX (ID `0x201`), nhập một số thập phân bình thường (ví dụ: `100`) rồi bấm nút Gửi (Send). Mạch sẽ nhớ con số 100 này làm mục tiêu.
4. **Cách chỉnh giờ khi đồng hồ sai:** Tìm đến khung CAN TX (ID `0x203`), nhập đúng thứ tự: *Ngày(1->31), Tháng(1->12), NăM(0->99), Thứ(1->7)*. Khung giờ nhập *Giờ(0->23), Phút(0->59), Giây(0->59)* (Dưới dạng số thập phân thông thường), rồi bấm Gửi (Send). Mạch sẽ nhận giờ lập tức, hết nhấp nháy chữ `----`.
5. **Cách đặt số đếm bất kỳ:** Tìm khung CAN TX (ID `0x202`), nhập một số bất kỳ (ví dụ: `50`), bấm Gửi, mạch sẽ lập tức nhảy số đếm về 50.

6. **Thay đổi tính năng của Thẻ RFID (Nạp lệnh mới vào thẻ):**
   - Mở khung CAN TX **`ID 0x204 (Cai_Dat_Tinh_Nang_RFID)`**.
   - Tại cột **Value**, nhập 1 trong các số sau tùy theo chức năng muốn cài:
     - `1`: Bật Relay (`BAT_RELAY`)
     - `2`: Đảo Relay (`DAO_RELAY`)
     - `3`: Phát nhạc (`PHAT_NHAC`)
     - `4`: Reset đếm (`RESET_DEM`)
   - Bấm Gửi. Mạch sẽ kêu **2 bíp ngắn dồn dập (50ms)** báo hiệu đang ở chế độ chờ ghi thẻ. Lúc này bạn quẹt thẻ từ vào mâm đọc, mạch sẽ nạp lệnh mới vào thẻ. (Quẹt xong sẽ có tiếng bíp dài báo thành công).
   *(Lưu ý : Để có thể thay đổi tính năng thẻ thì yêu cầu mật khẩu (key A) của thẻ phải là **0x11 0x22 0x33 0x44 0x55 0x66**)
7. **Đổi mật khẩu / Xóa trắng thẻ RFID:**
   - Mở khung CAN TX **`ID 0x205 (Cai_Dat_Mat_Khau_The)`**.
   - Cột **Lenh_Dieu_Khien (Byte 0):** Nhập `0` nếu muốn Xóa tính năng của thẻ (đưa về mặc định), hoặc nhập `1` nếu muốn Đổi mật khẩu mới.
   - **Cách điền Mật khẩu mới:**
     - Nếu bạn muốn nhập mật khẩu bằng hệ **Hexa**, hãy click đúp chuột trực tiếp vào khung **Data** (chữ số ngoài cùng) và sửa trực tiếp chuỗi Hex (Ví dụ: `01 0F FF FF FF FF FF` rồi ấn Enter). 
     - Nếu nhập từng cột Value, hãy quy đổi ra số Thập phân (Ví dụ `0xFF` thì gõ là `255`, `0x0F` thì gõ là `15`). Cangaroo không hiểu chữ "F" nếu gõ chay.
   - Bấm Gửi. Mạch kêu **2 bíp dồn dập** chờ thẻ. Quẹt thẻ để hoàn tất quá trình đổi mật khẩu.
    *(Lưu ý : Để có thể đổi mật khẩu mới cho thẻ cần biết mật khẩu hiện tại của thẻ)*
8. **Cấp mật khẩu tạm thời (Để ghi/đọc thẻ lạ):**
   - Nếu cầm một chiếc thẻ đã bị đổi mật khẩu ở nơi khác (Không phải mật khẩu mặc định của mạch), sẽ không thể đọc hay ghi đè nó. 
   - Lúc này mở khung CAN TX **`ID 0x206 (Mat_Khau_Tam_Thoi)`**, nhập mật khẩu đang dùng của chiếc thẻ đó vào 6 byte. Bấm Gửi.
   - Mạch sẽ lưu mật khẩu này vào bộ nhớ tạm để cho phép bạn đọc/xóa/đổi mật khẩu chiếc thẻ lạ đó trong lần quẹt tiếp theo.

9. **Quan sát Dữ liệu Mạch gửi lên PC (Monitor):**
   - Chuyển sang bảng **CAN RX** (Hoặc tab Monitor) để xem mạch báo cáo trạng thái theo thời gian thực:
   - **`ID 0x101`**: Thông báo có thẻ vừa quẹt. Xem được mã ID của thẻ (5 byte đầu) và trạng thái đọc thẻ (Byte cuối: `0` là Đọc thành công, `1-3` là có lỗi).
   - **`ID 0x102`**: Gửi liên tục thông số Số đếm hiện tại và Mục tiêu. (Bạn có thể thấy số đếm nhảy lên ngay lập tức trên máy tính khi che tay qua cảm biến).
   - **`ID 0x103`**: Gửi liên tục Thời gian thực (giờ,phút,giây,ngày,tháng,năm,thứ) từ vi điều khiển để bạn đồng bộ.
   - **`ID 0x104 & 0x105`**: Chứa chuỗi ký tự dữ liệu thô mạch đọc được từ Thẻ RFID khi bạn quẹt thẻ.

---

## 3. HƯỚNG DẪN DÀNH CHO LẬP TRÌNH VIÊN (API CÁC THƯ VIỆN)

Source code được viết bằng ngôn ngữ C++ và chia thành các Class (module) độc lập cho từng chức năng. Các hàm được lập trình để chạy đa nhiệm (không dùng lệnh chờ delay), giúp mạch xử lý cùng lúc nhiều việc mà không bị đơ/treo.

**Lưu ý khi mang thư viện sang dòng chip STM32 khác:**
Bộ thư viện này sử dụng lõi **HAL** của ST. Mặc định nó đang dùng cho dòng chip **STM32F1** 

Dưới đây là tài liệu chi tiết cách khai báo và sử dụng các thư viện:

### 3.1. Thư viện Còi Bíp (`Buzzer.h` / `Buzzer.cpp`)
Hệ thống còi bíp được thiết kế đặc biệt với hàng đợi thông minh (Queue). Nó không sử dụng `HAL_Delay` nên bạn có thể gọi tiếng bíp ở bất cứ đâu mà không sợ làm chậm chương trình.
- **Khai báo:** `Buzzer buzzer(GPIO_Port, GPIO_Pin);` (Ví dụ: `Buzzer buzzer(GPIOA, GPIO_PIN_4);`).
- **Khởi tạo:** `buzzer.init();` (Bắt buộc gọi ở hàm `setup`/trước vòng lặp `while`). Hàm này tự động cấu hình chân thành Output.
- **Sử dụng:**
  - `buzzer.beep(số_lần, thời_gian_kêu_ms, thời_gian_nghỉ_ms, [instant]);`
  - *Ví dụ 1:* `buzzer.beep(3, 100, 100);` -> Kêu 3 lần, mỗi lần 100ms, nghỉ 100ms. Lệnh này sẽ được đưa vào hàng đợi chờ phát.
  - *Ví dụ 2:* `buzzer.beep(1, 100, 0, true);` -> Thêm cờ `true` sẽ ép còi xóa hàng chờ và **phát ngay lập tức** (thường dùng cho thao tác quẹt thẻ để tăng cảm giác phản hồi nhanh).
- **Lưu ý Cốt lõi:** Bắt buộc phải đặt hàm `buzzer.update();` vào trong ngắt thời gian thực (ví dụ `SysTick_Handler` ở file `stm32f1xx_it.c` hoặc ngắt Timer định kỳ 1ms). Nếu không gọi hàm này, còi sẽ không bao giờ kêu vì không có biến nào đếm thời gian trôi qua.

### 3.2. Thư viện Phát nhạc (`Mp3.h` / `Mp3.cpp` - Chip VS1011e)
Giao tiếp với chip giải mã âm thanh VS1011e qua SPI. 
- **Khai báo:** `Mp3 mp3(&hspi2, XRESET_Port, XRESET_Pin, DREQ_Port, DREQ_Pin, XDCS_Port, XDCS_Pin, XCS_Port, XCS_Pin);`
  *(Truyền vào con trỏ khối SPI và từng cặp Cổng/Chân điều khiển của chip. Dễ dàng mang sang project khác chỉ việc đổi tên chân).* 
- **Sự phụ thuộc vào Thẻ nhớ (FatFs):** Thư viện Mp3 không tự quản lý thẻ nhớ. Nó bắt buộc phải dùng chung hệ thống tệp tin FatFs (ChaN) với module `SdCard`. Nghĩa là bạn PHẢI gọi hàm `sdCard.initialize()` (hoặc `f_mount()`) thành công trước khi gọi bất kỳ lệnh nào của Mp3. Nếu chưa mount được thẻ nhớ, hàm `mp3.play()` sẽ luôn báo lỗi trả về `false`.
- **Khởi tạo:** `mp3.init();` (Bắt đầu giao tiếp SPI, Reset chip VS1011e và nạp cấu hình âm lượng).
- **Sử dụng:** `mp3.play("TEN_FILE.MP3");`
  - Truyền vào tên file định dạng chuẩn (Ví dụ: `"DAT.MP3"`). Hàm trả về `true` (thành công) và `false` (lỗi thẻ nhớ, không thấy file, v.v.).
- **Tính năng Non-blocking:** Sự phụ thuộc của hàm `mp3.process()`: Hàm này sẽ dùng lệnh `f_read()` của FatFs để rút 32 byte từ thẻ nhớ rồi bơm qua SPI. Bạn phải liên tục gọi hàm `mp3.process();` trong vòng lặp `while (1)`. Nó chỉ thực sự chạy khi hàm `mp3.play()` trước đó đã mở file thành công, hễ chân DREQ của chip rảnh là nó bơm dữ liệu.
- **Tính năng Hot-plug:** Hỗ trợ lưu trạng thái `mp3.saveState();` (Ghi nhớ bài hát và vị trí đang phát) và khôi phục `mp3.resumeState();` để tự động hát nối tiếp khi cắm rút thẻ nhớ sống.

### 3.3. Thư viện Thẻ nhớ SD (`SdCard.h` / `SdCard.cpp`)
Chịu trách nhiệm giao tiếp vật lý SPI với thẻ nhớ, nhường quyền cho FatFs quản lý tệp tin.
- **Khai báo:** `SdCard sdCard(SPI_TypeDef, CS_Port, CS_Pin);` (Ví dụ: `SdCard sdCard(SPI1, GPIOC, GPIO_PIN_8);` - Lưu ý dùng con trỏ thanh ghi SPI của phần cứng, VD: `SPI1`, không phải HAL Handle `&hspi1`).
- **Khởi tạo:** `sdCard.init();` (Cấu hình chân CS thành Output cấp cao).
- **Kích hoạt thẻ:** `uint8_t stat = sdCard.initialize();`
  - Hàm này kích hoạt lệnh `f_mount()`. Trả về `0` (hoặc `FR_OK`) nếu thành công. Phải gọi hàm này mỗi khi cắm thẻ vào mạch.

### 3.4. Thư viện Thẻ Từ RFID (`Rfid.h` / `Rfid.cpp` - Chip RC522)
Sử dụng thư viện điều khiển MFRC522 qua SPI chuyên dụng, đã tối ưu tốc độ đọc.
- **Khai báo:** `Rfid rfid(SPI_TypeDef, CS_Port, CS_Pin, RST_Port, RST_Pin);` (Dùng thanh ghi SPI phần cứng, chân CS và chân Reset).
- **Khởi tạo:** `rfid.init();` (Khởi động chip, thiết lập tần số Anten, bật sóng).
- **Quy trình Đọc/Ghi thẻ chuẩn:**
  1. `rfid.checkCard()`: Trả về `true` nếu phát hiện có thẻ tiến vào vùng sóng từ.
  2. `rfid.readCardSerial(uid_mang_5_byte)`: Đọc mã ID (UID) của thẻ để nhận diện.
  3. `rfid.selectTag(uid)`: Khóa mục tiêu vào thẻ vừa đọc để làm việc sâu hơn.
  4. Xác thực Key A bằng hàm `rfid.authenticate(MFRC522_CMD_MF_AUTH_KEY_A, block_can_doc, mat_khau_6_byte, uid)`. Bắt buộc phải đúng mật khẩu của thẻ.
  5. Đọc dữ liệu: `rfid.MIFARE_Read(block_can_doc, mang_chua_du_lieu, &kich_thuoc)`.
  6. Ghi dữ liệu: `rfid.MIFARE_Write(block_can_ghi, mang_du_lieu_16_byte)`.
  7. Lệnh Ngủ: Bắt buộc gọi `rfid.PICC_HaltA();` cuối cùng để ép thẻ ngủ, tránh hiện tượng mạch đọc lặp đi lặp lại 1 thẻ liên tục.
- **Lưu ý:** Chip RC522 rất nhạy cảm với sụt áp nguồn. Nếu phần cứng bị cắm rút sống (hot-plug), chip sẽ bị treo do rớt sóng SPI. Phải khởi động lại (Reset) mạch để hàm `rfid.init()` chạy lại.

### 3.5. Thư viện Đồng Hồ Thực Tế (`Rtc.h` / `Rtc.cpp` - Chip DS1307)
Giao tiếp qua I2C (hỗ trợ mô phỏng Bitbang I2C) để duy trì thời gian bằng pin CMOS.
- **Khởi tạo:** `rtc.init();` (Cấu hình chân SCL, SDA).
- **Cách dùng:** 
  - `rtc.getTime(&hour, &min, &sec);` và `rtc.getDate(&dow, &date, &month, &year);`
  - `rtc.setTime(hour, min, sec);` và `rtc.setDate(dow, date, month, year);`
- **Tính năng nhận diện mất pin (Bản lề):** Hàm `rtc.isTimeLost()` kiểm tra bit thứ 7 (Clock Halt - CH) của thanh ghi giây. Nếu mạch hết pin CMOS, IC mất dao động, bit này nhảy lên 1. Lập trình viên gọi hàm này để biết lúc nào cần báo lỗi `----` ra màn hình.

### 3.6. Thư viện Màn Hình LED (`Led7Segment.h` / `Led7Segment.cpp` - Chip CD4094)
Màn hình LED 4 số lai ghép IC ghi dịch (Shift Register), sử dụng giao thức 3 dây.
- **Khai báo:** `Led7Segment dp(DATA_Port, DATA_Pin, CLK_Port, CLK_Pin, STROBE_Port, STROBE_Pin);`
- **Cách dùng cơ bản:**
  - `dp.setValue(giá_trị);`: Gán con số bạn muốn hiển thị lên màn hình (tối đa 9999).
  - `dp.setColon(true / false);`: Bật hoặc Tắt dấu 2 chấm ở giữa màn hình.
  - `dp.update(chế_độ_thời_gian);`: Đẩy dữ liệu ra IC ghi dịch để hiển thị. 
    - Truyền `dp.update(false)`: mạch tự động xóa các số 0 vô nghĩa ở đầu (Ví dụ: `0050` hiện là `50` - dùng cho đếm sản phẩm).
    - Truyền `dp.update(true)`: mạch giữ nguyên số 0 ở đầu (Ví dụ: hiện `08:30` - dùng cho xem giờ).
  - `dp.showDashes();`: Hiển thị dải ngang `----` để báo lỗi.

### 3.7. Mạng CAN Bus (`CanBus.h` / `CanBus.cpp`)
Giao tiếp công nghiệp siêu ổn định. Lớp Wrapper này giúp che giấu đi sự phức tạp của HAL.
- **Khai báo:** `CanBus myCan(&hcan);` (Truyền vào con trỏ CAN Handle, ví dụ CAN1).
- **Khởi tạo:** `myCan.init();` 
  - Hàm này bên trong đã tự động khởi tạo Bộ lọc (Filter) để nhận tất cả các ID (Mask = 0).
  - Nó thiết lập cờ `AutoBusOff = ENABLE` (Rất quan trọng để tự phục hồi mạng khi dây đứt).
- **Cách dùng:**
  - `myCan.sendFrame(id, mang_du_lieu, do_dai_byte);`: Trả về `true` nếu thành công. Tự động set Standard ID.
  - `myCan.readFrame(&id, mang_du_lieu, &do_dai_byte);`: Đặt trong vòng lặp chính. Nếu có dữ liệu trong FIFO, nó sẽ gắp ra và trả về `true`.
- **Lưu ý Data Endianness:** Chuẩn truyền của phần mềm PC (như Cangaroo / DBC) thường gửi biến 16-bit ở định dạng **Little-Endian**. Khi bóc 2 byte CAN thành 1 biến `uint16_t` trong C++, luôn dùng: `uint16_t value = data[0] | (data[1] << 8);`

### 3.8. Cảm biến và Rơ le (`Sensor.h` / `Relay.h`)
- **Relay (Công tắc):**
  - Khai báo: `Relay relay(GPIO_Port, GPIO_Pin);`
  - Các hàm: `relay.init()`, `relay.on()`, `relay.off()`, `relay.toggle()`, `relay.isOn()`.
- **Sensor (Cảm biến Đếm):**
  - Khai báo: `Sensor sensor(GPIO_Port, GPIO_Pin);`
  - Được cấu hình sử dụng **Ngắt Ngoài (EXTI)** để đếm sản phẩm ngay lập tức mà không làm trễ vòng lặp chính.
  - Hàm `Sensor::handleInterrupt(extiPin)` phải được gọi ở bên trong hàm ngắt `HAL_GPIO_EXTI_Callback()` (trong file `stm32f1xx_it.c`).
  - **Thuật toán lọc nhiễu (Debounce):** Được tích hợp cứng bên trong hàm ngắt. Thuật toán sẽ liên tục gọi hàm lấy thời gian thực `SystemTimer::getTick()`. Nếu 2 lần ngắt (2 lần báo có sản phẩm) xảy ra cách nhau dưới 20 mili-giây, nó sẽ tự động vứt bỏ tín hiệu thứ 2 vì phân tích đó chỉ là nhiễu phần cứng (rung nút bấm cơ học hoặc cảm biến quang dội sáng). Tính năng này chống lỗi đếm nhảy cóc cực kỳ hiệu quả.
  - **Chế độ Chẩn đoán (Diagnostic Mode):** Khi gọi hàm `enableDiagnosticMode(true)`, mạch sẽ kích hoạt thêm ngắt cạnh lên (Rising Edge). Chế độ này dùng để đo chính xác thời gian (mili-giây) sản phẩm che cảm biến và thời gian trống giữa 2 sản phẩm liên tiếp. Lập trình viên có thể lấy dữ liệu này thông qua hàm `getDiagnosticData()` để gửi lên PC, giúp phân tích và tinh chỉnh thuật toán khi băng chuyền gặp lỗi đếm sai (ví dụ: chai nhựa trong suốt làm cảm biến dội tín hiệu nhiều lần).
