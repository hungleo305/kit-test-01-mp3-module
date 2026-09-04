# TÀI LIỆU HƯỚNG DẪN SỬ DỤNG VÀ PHÁT TRIỂN FIRMWARE - KIT VNA

Dự án mạch Kit STM32F103RCT6 điều khiển đa năng giao tiếp CAN Bus, tích hợp đọc thẻ RFID, thẻ nhớ SD (phát nhạc MP3), cảm biến đếm sản phẩm, và LED 7 đoạn.

---

## 1. MÔI TRƯỜNG & PHẦN MỀM SỬ DỤNG
Để lập trình, biên dịch và giao tiếp với mạch, dự án sử dụng các phần mềm chuyên dụng sau:

- **STM32CubeMX (Phiên bản 6.17.0):** Công cụ sinh code tự động (C code) và cấu hình phần cứng (Clock, GPIO, SPI, I2C, CAN...). File cấu hình dự án là `Firmware.ioc`. Khi muốn thay đổi sơ đồ chân mạch, hãy dùng CubeMX để Generate Code ban đầu.
- **IAR Embedded Workbench (Phiên bản 8.40.1.21539):** Trình biên dịch (Compiler) và Môi trường phát triển (IDE) chính để viết code C++ và nạp chương trình. Sử dụng Workspace `Project.eww` nằm trong thư mục `EWARM/`. Mã nguồn của dự án được tối ưu rất sâu trên IAR, tận dụng trình biên dịch DLib và kiến trúc tối ưu bộ nhớ.
- **Cangaroo (Phiên bản 0.6.3):** Phần mềm giả lập màn hình HMI/PC mã nguồn mở. Dùng để gửi và nhận tín hiệu mạng CAN Bus giữa máy tính và mạch. Hỗ trợ import Database trực tiếp qua file `.dbc` để hiển thị tiếng Việt thay cho các mã Hexa khó đọc.

---

## 2. CẤU TRÚC THƯ MỤC DỰ ÁN

```Text
kit-test-01-mp3-module/
├── Core/
│   ├── Inc/                # Header thư viện C++ (.h)
│   │   ├── Buzzer.h        # Còi chip 
│   │   ├── CanBus.h        # Mạng CAN 
│   │   ├── Led7Segment.h   # LED 7 đoạn CD4094
│   │   ├── Mp3.h           # Phát nhạc VS1101E
│   │   ├── Relay.h         # Công tắc
│   │   ├── Rfid.h          # Thẻ RC522
│   │   ├── Rtc.h           # Thời gian DS1307
│   │   ├── SdCard.h        # Thẻ nhớ
│   │   ├── Sensor.h        # Cảm biến đếm
│   └── Src/                # Source code C/C++ (.cpp / .c)
│       ├── main.cpp        # Chương trình chính
│       ├── Sensor.cpp      
│       ├── stm32f1xx_it.c  # File chứa các ngắt hệ thống (SysTick, HardFault)
│       └── *.cpp           # (Các file mã nguồn tương ứng cho thư viện trong thư mục Inc/)
├── Drivers/                # Thư viện phần cứng (HAL) chính thức của hãng ST
├── EWARM/                  # Không gian làm việc (Workspace) của phần mềm IAR
│   └── Project.eww         # Mở file này bằng IAR để lập trình & biên dịch
├── FATFS/                  # Thư viện File System (FatFs) dùng để đọc thẻ SD
├── Middlewares/            # Các mã nguồn phần mềm trung gian liên quan đến FatFs
├── Firmware.ioc            # File cấu hình đồ họa hệ thống cho STM32CubeMX
├── kit_vna_can.dbc         # Database ánh xạ dữ liệu CAN cho Cangaroo
└── README.md               # Tài liệu đang đọc
```

---

## 3. HƯỚNG DẪN SỬ DỤNG DÀNH CHO NGƯỜI DÙNG

Phần này hướng dẫn chi tiết cách vận hành mạch từ lúc cắm điện đến lúc sử dụng các tính năng. 
### 3.1. Cắm điện và Khởi động
- **Cắm điện:** Ngay khi vừa cấp nguồn, mạch sẽ khởi động hệ thống và kêu **2 tiếng Bíp** để báo hiệu đã có điện và sẵn sàng.
- **Đèn LED Onboard :** Đèn LED nhỏ trên thân mạch sẽ chớp nháy liên tục rất nhanh (chu kỳ 0.1 giây sáng / 0.1 giây tắt - tức là chớp 5 lần/giây). Đèn này nháy đều đặn chứng tỏ  vi xử lý đang chạy ổn định, vòng lặp hoạt động không bị treo .
- **Màn hình LED 7 đoạn:** Sẽ bắt đầu tự động luân phiên hiển thị giữa 2 thông tin (mỗi thông tin hiển thị 5 giây):
  1. **Số lượng sản phẩm đã đếm được:** Dấu chấm ở giữa màn hình sẽ **TẮT**.
  2. **Giờ hiện tại (Giờ:Phút):** Dấu chấm ở giữa màn hình sẽ **NHẤP NHÁY** theo từng giây.

### 3.2. Tín hiệu Còi (Buzzer) và Cảnh báo
Hệ thống còi bíp đã được tinh chỉnh để chạy hoàn toàn đa nhiệm (có hàng chờ) nên các tiếng bíp sẽ lần lượt phát ra mượt mà, không bao giờ bị dính hoặc nuốt tiếng của nhau. 

Mạch được quy định phản hồi các thao tác thông qua số lượng tiếng bíp như sau:

| Âm thanh | Tình huống | Ý nghĩa chi tiết |
| :--- | :--- | :--- |
| **1 Bíp ngắn** | **Thành công** | - Có sản phẩm chạy qua mắt cảm biến.<br>- Nhận diện thành công có thẻ RFID quẹt qua.<br>- Cập nhật thành công cài đặt mới từ phần mềm máy tính (CAN Bus: Mục tiêu, Bộ đếm, Xem giờ). |
| **2 Bíp ngắn** | **Lỗi Dữ liệu / Cảnh báo nhẹ** | - **Khởi động:** Báo hiệu mạch vừa khởi động/cấp điện thành công.<br>- **Thẻ SD (Hot-plug):** Cắm thẻ SD thành công khi mạch đang chạy.<br>- **CAN Bus:** Gửi cài đặt Mục tiêu / Số đếm lớn hơn `9999` (Vượt quá giới hạn màn hình LED).<br>- **Quẹt Thẻ (RFID):** Sai mật khẩu thẻ.<br>- **Quẹt Thẻ (RFID):** Lỗi giao tiếp với chip nhớ trong thẻ (không đọc được Block).<br>- **Quẹt Thẻ (RFID):** Dữ liệu trong thẻ bị sai lệch (VD: Không chứa các lệnh hợp lệ như `RESET_DEM`, `BAT_RELAY`...).<br>- **Quẹt Thẻ (RFID) Thao tác thừa:** Ví dụ thẻ mang lệnh bắt "Bật Relay" nhưng Relay đang bật sẵn, hoặc thẻ mang lệnh "Reset bộ đếm" nhưng số đếm đang là 0 sẵn rồi. |
| **3 Bíp ngắn** | **Lỗi Phần cứng / Module MP3** | - **Lỗi Thẻ Nhớ SD:** Thẻ nhớ bị lỗi định dạng (yêu cầu FAT32), chưa cắm thẻ nhớ, hoặc khe cắm hỏng tiếp xúc phần cứng.<br>- **Lỗi Module MP3:** Module âm thanh MP3 bị treo không phản hồi, hoặc thẻ nhớ không có tệp `.MP3` tương ứng với hành động. |
| **1 Bíp Dài**<br>*(Nửa giây)* | **Lỗi Mất Mạng CAN** | - Mạch cảnh báo phần cứng phát hiện đường truyền mạng CAN Bus đang bị đứt cáp hoặc lỗi bộ chuyển đổi máy tính. |

**Tín hiệu Cảnh báo trên Màn hình LED:**
- **Lỗi đứt cáp mạng CAN:** Nếu mạch phát hiện mạng CAN bị đứt, ở chế độ đếm số, **dấu chấm ở giữa màn hình sẽ nhấp nháy liên tục rất nhanh (5 lần/giây)** để cảnh báo dữ liệu không gửi được lên máy tính. Ngay khi khôi phục lại kết nối canbus, phần cứng sẽ tự khôi phục và dấu chấm tự động ngừng nhấp nháy (hệ thống về trạng thái bình thường).
- **Lỗi mất thời gian thực (Hết pin RTC):** Nếu mạch bị mất thời gian thực. Khi hiển thị chế độ Xem giờ, thay vì hiện `Giờ:Phút`, màn hình sẽ hiện chữ `----` nhấp nháy mãi mãi để báo lỗi. Lúc này cần nạp lại thời gian qua CAN Bus (Cangaroo).

### 3.3. Hướng dẫn đếm sản phẩm bằng Cảm biến
- **Cách đếm:** Khi có sản phẩm chạy ngang qua mắt cảm biến, còi kêu 1 tiếng **Bíp** và số trên màn hình tăng lên 1. 
- **Khi màn hình đang hiển thị chế độ "Xem giờ":** Nếu cảm biến bắt được sản phẩm, màn hình sẽ **lập tức nhảy về chế độ Số Đếm** để dễ dàng theo dõi.
- **Đạt mục tiêu:** Khi số lượng sản phẩm đếm được bằng với con số "Mục tiêu" (có thể cài đặt qua máy tính bằng canbus), loa sẽ tự động phát âm thanh "Đã đạt mục tiêu" (tên file là `DAT.MP3`).

### 3.4. Hướng dẫn sử dụng Thẻ Nhớ MP3 (Thẻ SD)
- **Chuẩn bị thẻ:** Rút thẻ ra cắm vào máy tính, Format thẻ với định dạng **FAT32**. Sau đó copy các file âm thanh (đuôi file là `.mp3`) trực tiếp vào thẻ, không bỏ vào thư mục nào cả. 
- **Rút/Cắm thẻ khi đang chạy (Hot-plug):** Nếu đang phát âm thanh mà vô tình rút thẻ ra -> Mạch sẽ kêu **1 tiếng bíp**, đồng thời tự động ngắt amply để loa không bị nổ lụp bụp và lưu lại vị trí bài hát đang nghe dở. Khi cắm thẻ lại thành công -> Mạch sẽ kêu **2 tiếng bíp** nếu kết nối thành công, đồng thời **màn hình LED 7 đoạn sẽ hiển thị thời gian kết nối thẻ (tính bằng mili-giây) trong vòng 3 giây**, sau đó mạch tự động phát tiếp đoạn âm thanh vừa bị ngắt.

### 3.5. Hướng dẫn Quẹt thẻ Thao tác (RFID)
Mạch đi kèm bộ thẻ từ. Quẹt các thẻ này vào anten (miếng vuông in chữ RC522) để ra lệnh cho mạch làm việc:
- Quẹt thẻ **RESET**: Mạch tự động xóa số đếm sản phẩm về 0 và loa phát bài `RESET.MP3`.
- Quẹt thẻ **BAT**: Bật công tắc điện (Relay) trên mạch, loa phát bài `BAT.MP3`.
- Quẹt thẻ **DAO**: Đổi ngược trạng thái công tắc (Đang bật thành tắt, đang tắt thành bật), loa phát bài `DAO.MP3`.
- Quẹt thẻ **PHAT_NHAC**: Ra lệnh cho loa phát một bài nhạc giải trí tên là `MUSIC.MP3`.
*( Có thể quẹt liên tục nhiều thẻ khác nhau, các âm thanh sẽ tự động xếp hàng và phát lần lượt).*

### 3.6. Hướng dẫn dùng phần mềm máy tính (Cangaroo qua CAN Bus)
Cắm mạch vào máy tính qua bộ chuyển đổi CAN. Mở phần mềm **Cangaroo** trên máy tính:

**Cấu hình kết nối phần cứng:**
1. Trên thanh Menu Cangaroo, vào **Measurement -> Measurement Setup...**
2. Tại mục **Interfaces**, chọn cổng COM tương ứng của bộ chuyển đổi CAN (ví dụ: `COM6`).
3. Đặt **Bitrate** là `500000` (đây là tốc độ baud mặc định của firmware).
4. Bấm **Start Measurement** (nút màu xanh hình nút Play) để bắt đầu quét mạng CAN.

![Cấu hình Cangaroo](docs/cangaroo_setup.png)

**Sử dụng Cangaroo để Gửi/Nhận dữ liệu:**
1. Vào thanh Menu **View -> Generator** để mở bảng Gửi lệnh (Generator View).
2. Tại bảng **Available Messages (DBC)**, bấm nút **Add DBC Message**, tìm file `kit_vna_can.dbc` (có sẵn trong thư mục dự án) để Load vào phần mềm.
3. Lúc này toàn bộ các ID kỹ thuật sẽ tự động biến thành tiếng Việt (Ví dụ: *Thoi_Gian_Thuc_RTC, Muc_Tieu...*).

![Giao diện Generator](docs/cangaroo_generator.png)

4. **Cách đổi số lượng Mục Tiêu:** Tìm đến khung CAN TX (ID `0x201`), nhập một số thập phân bình thường (ví dụ: `100`) rồi bấm nút Gửi (Send). Mạch sẽ nhớ con số 100 này làm mục tiêu.
5. **Cách chỉnh giờ khi đồng hồ sai:** Tìm đến khung CAN TX (ID `0x203`), nhập đúng thứ tự: *Ngày(1->31), Tháng(1->12), NăM(0->99), Thứ(1->7)*. Khung giờ nhập *Giờ(0->23), Phút(0->59), Giây(0->59)* (Dưới dạng số thập phân thông thường), rồi bấm Gửi (Send). Mạch sẽ nhận giờ lập tức, hết nhấp nháy chữ `----`.
6. **Cách đặt số đếm bất kỳ:** Tìm khung CAN TX (ID `0x202`), nhập một số bất kỳ (ví dụ: `50`), bấm Gửi, mạch sẽ lập tức nhảy số đếm về 50.

7. **Thay đổi tính năng của Thẻ RFID (Nạp lệnh mới vào thẻ):**
   - Mở khung CAN TX **`ID 0x204 (Cai_Dat_Tinh_Nang_RFID)`**.
   - Tại cột **Value**, nhập 1 trong các số sau tùy theo chức năng muốn cài:
     - `1`: Reset đếm (`RESET_DEM`)
     - `2`: Bật Relay (`BAT_RELAY`)
     - `3`: Đảo Relay (`DAO_RELAY`)
     - `4`: Phát nhạc (`PHAT_NHAC`)
   - Bấm Gửi. Mạch sẽ kêu **2 bíp ngắn dồn dập (50ms)** báo hiệu đang ở chế độ chờ ghi thẻ. Lúc này quẹt thẻ từ vào anten, mạch sẽ nạp lệnh mới vào thẻ. (Quẹt xong sẽ có tiếng bíp dài báo thành công).
   *(Lưu ý : Để có thể thay đổi tính năng thẻ thì yêu cầu mật khẩu (key A) của thẻ phải là **0x11 0x22 0x33 0x44 0x55 0x66**)
8. **Đổi mật khẩu / Xóa trắng thẻ RFID:**
   - Mở khung CAN TX **`ID 0x205 (Cai_Dat_Mat_Khau_The)`**.
   - Cột **Lenh_Dieu_Khien (Byte 0):** Nhập `0` nếu muốn Xóa tính năng của thẻ , hoặc nhập `1` nếu muốn Đổi mật khẩu mới.
   - **Cách điền Mật khẩu mới:**
     - Nếu muốn nhập mật khẩu bằng hệ **Hexa**, hãy click đúp chuột trực tiếp vào khung **Data** (chữ số ngoài cùng) và sửa trực tiếp chuỗi Hex (Ví dụ: `01 0F FF FF FF FF FF` rồi ấn Enter). 
     - Nếu nhập từng cột Value, hãy quy đổi ra số Thập phân (Ví dụ `0xFF` thì gõ là `255`, `0x0F` thì gõ là `15`). 
   - Bấm Gửi. Mạch kêu **2 bíp dồn dập** chờ thẻ. Quẹt thẻ để hoàn tất quá trình đổi mật khẩu.
    *(Lưu ý : Để có thể đổi mật khẩu mới cho thẻ cần biết mật khẩu hiện tại của thẻ)*

9. **Cấp Chìa khóa Tạm thời :**
   - Theo mặc định, mạch sẽ tự động truy cập thẻ bằng mật khẩu (Key A) là `11 22 33 44 55 66`. Tuy nhiên, nếu thẻ có mật khẩu lạ, sẽ không thể thao tác Ghi lệnh (Bước 6) hay Xóa/Đổi mật khẩu (Bước 7) được nữa, mạch sẽ kêu 2 tiếng bíp dài báo sai mật khẩu.
   - Để khắc phục, cần mở khung CAN TX **`ID 0x206 (Cai_Dat_Mat_Khau_Tam_Thoi)`**.
   - Tại phần Data, nhập 6 byte mật khẩu hiện tại của thẻ (ví dụ `AA BB CC DD EE FF`), rồi bấm Gửi.
   - Mạch sẽ kêu **2 bíp ngắn dồn dập**. Ngay lúc này, mạch đã lưu chìa khóa tạm thời vào bộ nhớ RAM. Có thể tiến hành gửi lệnh `0x204` (Đổi tính năng) hoặc `0x205` (Xóa/Đổi mật khẩu) như bình thường. Chìa khóa tạm này sẽ được sử dụng cho đúng 1 lần quẹt thẻ kế tiếp.

10. **Quan sát Dữ liệu Mạch gửi lên PC (Monitor):**
   - Chuyển sang bảng **CAN RX** (Hoặc tab Monitor) để xem mạch báo cáo trạng thái theo thời gian thực:
   - **`ID 0x101`**: Thông báo có thẻ vừa quẹt. Xem được mã ID của thẻ (5 byte đầu) và trạng thái đọc thẻ (Byte cuối: `0` là Đọc thành công, `1-3` là có lỗi).
   - **`ID 0x102`**: Gửi liên tục thông số Số đếm hiện tại, Mục tiêu và Chu kỳ (số lần đếm được 10000 ). (Có thể thấy số đếm nhảy lên ngay lập tức trên máy tính khi che tay qua cảm biến).
   - **`ID 0x103`**: Gửi liên tục Thời gian thực (giờ,phút,giây,ngày,tháng,năm,thứ) từ vi điều khiển để đồng bộ.
   - **`ID 0x104 & 0x105`**: Chứa chuỗi ký tự dữ liệu (16 byte) mạch đọc được từ Thẻ RFID khi quẹt thẻ.
   - **`ID 0x124`**: (Chế độ Diagnostic) Liên tục cập nhật thời gian che và thời gian khoảng trống của cảm biến (ms). Dùng để gỡ lỗi khi có nhiễu.
   - **`ID 0x125`**: Báo cáo thời gian khởi tạo thẻ nhớ SD (Mount Time - tính bằng ms) ngay khi thẻ được cắm nóng.

---


## 4. HƯỚNG DẪN DÀNH CHO LẬP TRÌNH VIÊN (Dễ Hiểu Nhất)

Phần này dành cho những ai muốn chỉnh sửa mã nguồn (code) của dự án. Đừng lo lắng nếu bạn mới bắt đầu, mọi thứ đã được chuẩn bị sẵn ở dạng "Copy & Paste".

### 4.1. Bắt Đầu Nhanh (Mở Code và Nạp vào mạch)

**Bước 1: Mở phần mềm lập trình**
- Cài đặt phần mềm **IAR Embedded Workbench**.
- Vào thư mục `EWARM`, click đúp chuột vào file **`Project.eww`** để mở dự án.
- Phía bên trái phần mềm, mở nhánh thư mục `Application/User/Core` và click đúp vào file `main.cpp` để bắt đầu viết code.

**Bước 2: Biên dịch (Kiểm tra lỗi)**
- Bấm nút **Make** (hoặc nhấn phím tắt `F7`). 
- Phần mềm sẽ dịch code C++ của bạn. Nếu bảng thông báo bên dưới hiện chữ `Errors: 0` là thành công.

**Bước 3: Nạp code xuống mạch**
- Cắm mạch vào máy tính qua cổng nạp ST-Link (Chân SWDIO, SWCLK).
- Bấm nút **Download and Debug** (hoặc `Ctrl + D`). Code sẽ bay thẳng vào chip.
- Bấm nút hình "Dấu X" đỏ để thoát chế độ Debug, sau đó mạch sẽ tự động chạy code mới.

---

### 4.2. Còi Chip (Buzzer)
Chỉ với 1 dòng code, mạch sẽ tự động đếm thời gian và phát ra tiếng bíp mà không làm treo hệ thống.

**Cách dùng trong `main.cpp`:**
```cpp
// Lệnh này kêu 3 lần, mỗi lần kêu 100ms, nghỉ 100ms
buzzer.beep(3, 100, 100); 

// Kêu 1 tiếng bíp ngắn (50ms)
buzzer.beep(1, 50, 0); 
```
*Lưu ý duy nhất: Luôn để hàm `buzzer.update();` nằm bên trong hàm `HAL_IncTick` (Ở cuối file `main.cpp`) để còi biết đếm giờ.*

---

### 4.3. Thẻ nhớ SD và Phát Nhạc MP3
Thư viện đã được làm cực kỳ đơn giản để bạn chỉ việc gọi hàm là nhạc tự kêu. Đã bao gồm tính năng tự động ghi nhớ bài hát nếu vô tình rút thẻ.

**Cách dùng:**
```cpp
// Phát 1 file âm thanh tên là XINCHAO.MP3
mp3.play("XINCHAO.MP3");

// Phát nối tiếp (Hệ thống sẽ tự chờ bài cũ hát xong mới hát bài mới)
mp3.play("SONG.MP3"); 

// Chỉnh âm lượng (0 = To nhất, 254 = Tắt tiếng)
mp3.setVolume(20, 20);
```
*Lưu ý: Bắt buộc phải đặt hàm `mp3.process()` bên trong vòng lặp `while(1)` để bộ vi xử lý liên tục đẩy nhạc từ thẻ nhớ sang chip âm thanh.*

---

### 4.4. Đọc Thẻ Từ (RFID RC522)
Không cần quan tâm thẻ bị mã hóa hay bảo mật ra sao. Code mẫu sau đây sẽ tự động phát hiện thẻ, mở khóa và rút dữ liệu ra cho bạn:

**Code mẫu bỏ vào `while(1)`:**
```cpp
// 1. Quét xem có ai quẹt thẻ không?
if (rfid.checkCard()) {
    uint8_t uid[5]; // Mảng chứa ID của thẻ
    
    // 2. Lấy số ID của thẻ
    if (rfid.readCardSerial(uid)) {
        rfid.selectTag(uid); 
        
        uint8_t mat_khau_mac_dinh[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
        
        // 3. Mở khóa và đọc nội dung ẩn trong thẻ
        if (rfid.PCD_Authenticate(0x60, 1, mat_khau_mac_dinh, uid) == 0) {
            uint8_t du_lieu[16]; 
            uint8_t do_dai = 16;
            
            // Lấy 16 chữ số giấu trong thẻ bỏ vào mảng "du_lieu"
            rfid.MIFARE_Read(1, du_lieu, &do_dai);
        }
        
        rfid.PICC_HaltA(); // Báo thẻ đi ngủ, quẹt xong rồi!
    }
}
```

---

### 4.5. Cảm biến Đếm và Công tắc (Relay)
Relay giúp bạn đóng/ngắt các thiết bị điện lớn (Ví dụ: Động cơ, đèn). Cảm biến giúp bạn biết khi nào có vật đi qua.

**Dùng Relay:**
```cpp
relay.on();     // Bật công tắc
relay.off();    // Tắt công tắc
relay.toggle(); // Đảo trạng thái (Đang bật thành tắt, đang tắt thành bật)
```

**Dùng Cảm biến (Chỉ dùng Callback, không cần dùng while 1):**
```cpp
// 1. Viết 1 hàm xử lý khi có vật cản
void Co_Nguoi_Di_Qua() {
    relay.on();      // Bật đèn
    so_luong++;      // Tăng số đếm
    buzzer.beep(1, 100, 0); // Kêu 1 tiếng bíp
}

int main() {
    // 2. Khai báo ở trên đầu hàm main()
    cam_bien.init(Sensor::FALLING_EDGE); // FALLING_EDGE: Kích hoạt khi tay che vào
    cam_bien.setCallback(Co_Nguoi_Di_Qua); // Gắn hàm ở trên vào cảm biến
    
    while(1) {
       // Không cần viết gì vào đây, cảm biến sẽ tự động chạy ngầm!
    }
}
```

---

### 4.6. Đồng Hồ Thời Gian Thực (RTC DS1307)
Dùng để lấy Giờ/Phút/Giây và Ngày/Tháng/Năm. Khi cúp điện đồng hồ vẫn chạy (Nhờ viên pin tròn CMOS trên mạch).

**Cách đọc giờ hiện tại:**
```cpp
uint8_t gio, phut, giay, ngay, thang, nam, thu;

// Lấy toàn bộ ngày tháng
rtc.getDate(&ngay, &thang, &nam, &thu);

// Lấy toàn bộ giờ phút
rtc.getTime(&gio, &phut, &giay);
```

**Cách chỉnh lại giờ:**
```cpp
// Chỉnh thành: Thứ 3, ngày 25/08/2026
rtc.setDate(25, 8, 26, 3); 

// Chỉnh thành: 14 giờ 30 phút 0 giây
rtc.setTime(14, 30, 0);
```

---

### 4.7. Mạng Cấp Máy Tính (CAN Bus)
Đây là thư viện giúp mạch nói chuyện với phần mềm **Cangaroo** trên máy tính qua 2 sợi dây điện (CAN H và CAN L). Thư viện tự động chạy ngầm, không bao giờ làm trễ chương trình chính.

**Cách Gửi số liệu từ mạch lên máy tính:**
Do dây mạng chỉ chở được từng Byte (từng số nhỏ < 255), nên nếu bạn muốn gửi 1 số lớn như `5000`, bạn phải cắt nó ra làm 2 khúc (Byte cao và Byte thấp).

```cpp
// Ví dụ muốn gửi số 5000 lên phần mềm máy tính:
uint16_t so_dem_lon = 5000;
uint8_t mang_chua_du_lieu[2];

// Cắt đôi số 5000 ra làm 2 phần nhỏ (ép kiểu)
mang_chua_du_lieu[0] = so_dem_lon & 0xFF;         // Phần đuôi
mang_chua_du_lieu[1] = (so_dem_lon >> 8) & 0xFF;  // Phần đầu

// Bắn lên máy tính (ID = 0x102, mảng dữ liệu, độ dài 2)
myCan.sendFrame(0x102, mang_chua_du_lieu, 2);
```

**Cách Nhận lệnh từ phần mềm máy tính (Bỏ vào ngắt RxFifo0 ở file `stm32f1xx_it.c`):**
```cpp
uint32_t ID_tu_may_tinh;
uint8_t du_lieu_nhan[8];
uint8_t do_dai;

// Rút dữ liệu khỏi mạng
myCan.readFrame(&ID_tu_may_tinh, du_lieu_nhan, &do_dai);

if (ID_tu_may_tinh == 0x201) {
    // Ráp 2 mảnh Byte (đuôi và đầu) lại thành 1 con số lớn như ban đầu
    uint16_t ket_qua = du_lieu_nhan[0] | (du_lieu_nhan[1] << 8);
    
    // Nếu bấm nút trên phần mềm Cangaroo, biến ket_qua sẽ có đúng số bạn vừa gõ!
}
```
