# TÀI LIỆU HƯỚNG DẪN SỬ DỤNG VÀ PHÁT TRIỂN FIRMWARE - KIT VNA

Dự án mạch Kit STM32F103RCT6 điều khiển đa năng giao tiếp CAN Bus, tích hợp đọc thẻ RFID, thẻ nhớ SD (phát nhạc MP3), cảm biến đếm sản phẩm, và LED 7 đoạn.

---

## 1. MÔI TRƯỜNG & PHẦN MỀM SỬ DỤNG
Để lập trình, biên dịch và giao tiếp với mạch, dự án sử dụng các phần mềm chuyên dụng sau:

- **STM32CubeMX (Phiên bản 6.17.0):** Công cụ sinh code tự động (C code) và cấu hình phần cứng (Clock, GPIO, SPI, I2C, CAN...). File cấu hình dự án là `Firmware.ioc`. Khi bạn thay đổi sơ đồ chân mạch, hãy dùng CubeMX để Generate Code ban đầu.
- **IAR Embedded Workbench (Phiên bản 8.40.1.21539):** Trình biên dịch (Compiler) và Môi trường phát triển (IDE) chính để viết code C++ và nạp chương trình. Sử dụng Workspace `Project.eww` nằm trong thư mục `EWARM/`. Mã nguồn của dự án được tối ưu rất sâu trên IAR, tận dụng trình biên dịch DLib và kiến trúc tối ưu bộ nhớ.
- **Cangaroo (Phiên bản 0.6.3):** Phần mềm giả lập màn hình HMI/PC mã nguồn mở. Dùng để gửi và nhận tín hiệu mạng CAN Bus giữa máy tính và mạch. Hỗ trợ import Database trực tiếp qua file `.dbc` để hiển thị tiếng Việt thay cho các mã Hexa khó đọc.

---

## 2. CẤU TRÚC THƯ MỤC DỰ ÁN

```Text
kit-test-01-mp3-module/
├── Core/
│   ├── Inc/                # Header thư viện C++ (.h)
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
│   └── Src/                # Source code C/C++ (.cpp / .c)
│       ├── main.cpp        # Vòng lặp chính, xử lý CAN và logic trung tâm
│       ├── Sensor.cpp      # Tích hợp sẵn ngắt EXTI (EXTI15_10_IRQHandler) để đọc cảm biến
│       ├── stm32f1xx_it.c  # File chứa các ngắt hệ thống (SysTick, HardFault)
│       └── *.cpp           # (Các file mã nguồn tương ứng cho thư viện trong thư mục Inc/)
├── Drivers/                # Thư viện phần cứng (HAL) chính thức của hãng ST
├── EWARM/                  # Không gian làm việc (Workspace) của phần mềm IAR
│   └── Project.eww         # Mở file này bằng IAR để lập trình & biên dịch
├── FATFS/                  # Thư viện File System (FatFs) dùng để đọc thẻ SD
├── Middlewares/            # Các mã nguồn phần mềm trung gian liên quan đến FatFs
├── Firmware.ioc            # File cấu hình đồ họa hệ thống cho STM32CubeMX
├── kit_vna_can.dbc         # Database ánh xạ dữ liệu CAN sang tiếng Việt (Cho Cangaroo)
└── README.md               # Tài liệu bạn đang đọc
```

---

## 3. HƯỚNG DẪN SỬ DỤNG DÀNH CHO NGƯỜI DÙNG

Phần này hướng dẫn chi tiết cách vận hành mạch từ lúc cắm điện đến lúc sử dụng các tính năng. Bạn chỉ cần làm theo các bước dưới đây.

### 4.1. Cắm điện và Khởi động
- **Cắm điện:** Ngay khi vừa cấp nguồn, mạch sẽ khởi động hệ thống và kêu **2 tiếng Bíp** để báo hiệu đã có điện và sẵn sàng.
- **Đèn LED Onboard :** Đèn LED nhỏ trên thân mạch sẽ chớp nháy liên tục rất nhanh (chu kỳ 0.1 giây sáng / 0.1 giây tắt - tức là chớp 5 lần/giây). Đèn này nháy đều đặn chứng tỏ  vi xử lý đang chạy cực kỳ trơn tru, vòng lặp hoạt động không bị treo .
- **Màn hình LED 7 đoạn:** Sẽ bắt đầu tự động luân phiên hiển thị giữa 2 thông tin (mỗi thông tin hiển thị 5 giây):
  1. **Số lượng sản phẩm đã đếm được:** Dấu chấm ở giữa màn hình sẽ **TẮT**.
  2. **Giờ hiện tại (Giờ:Phút):** Dấu chấm ở giữa màn hình sẽ **NHẤP NHÁY** theo từng giây.

### 4.2. Tín hiệu Còi (Buzzer) và Cảnh báo
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

### 4.3. Hướng dẫn đếm sản phẩm bằng Cảm biến
- **Cách đếm:** Khi có sản phẩm chạy ngang qua mắt cảm biến, còi kêu 1 tiếng **Bíp** và số trên màn hình tăng lên 1. 
- **Tính năng lấn át:** Nếu màn hình đang ở chế độ "Xem giờ" mà cảm biến bắt được sản phẩm, màn hình sẽ **lập tức nhảy về chế độ Số Đếm** để dễ dàng theo dõi.
- **Đạt mục tiêu:** Khi số lượng sản phẩm đếm được bằng với con số "Mục tiêu" (có thể cài đặt qua máy tính bằng canbus), loa sẽ tự động phát âm thanh "Đã đạt mục tiêu" (tên file là `DAT.MP3`).

### 4.4. Hướng dẫn sử dụng Thẻ Nhớ MP3 (Thẻ SD)
- **Chuẩn bị thẻ:** Rút thẻ ra cắm vào máy tính, Format thẻ với định dạng **FAT32**. Sau đó copy các file âm thanh (đuôi file là `.mp3`) trực tiếp vào thẻ, không bỏ vào thư mục nào cả. 
- **Rút/Cắm thẻ khi đang chạy (Hot-plug):** Nếu đang phát âm thanh mà vô tình rút thẻ ra -> Mạch lập tức tự động ngắt amply để loa không bị nổ lụp bụp, đồng thời tự động lưu lại vị trí bài hát đang nghe dở. Khi cắm thẻ lại -> Mạch sẽ tự động mount thẻ nhớ và **tự động hát tiếp** đoạn âm thanh vừa bị ngắt.

### 4.5. Hướng dẫn Quẹt thẻ Thao tác (RFID)
Mạch đi kèm bộ thẻ từ. Quẹt các thẻ này vào anten (miếng vuông in chữ RC522) để ra lệnh cho mạch làm việc:
- Quẹt thẻ **RESET**: Mạch tự động xóa số đếm sản phẩm về 0 và loa phát bài `RESET.MP3`.
- Quẹt thẻ **BAT**: Bật công tắc điện (Relay) trên mạch, loa phát bài `BAT.MP3`.
- Quẹt thẻ **DAO**: Đổi ngược trạng thái công tắc (Đang bật thành tắt, đang tắt thành bật), loa phát bài `DAO.MP3`.
- Quẹt thẻ **PHAT_NHAC**: Ra lệnh cho loa phát một bài nhạc giải trí tên là `MUSIC.MP3`.
*( Có thể quẹt liên tục nhiều thẻ khác nhau, các âm thanh sẽ tự động xếp hàng và phát lần lượt).*

### 4.6. Hướng dẫn dùng phần mềm máy tính (Cangaroo qua CAN Bus)
Bạn cắm mạch vào máy tính qua bộ chuyển đổi CAN. Mở phần mềm **Cangaroo** trên máy tính:
1. Load file có sẵn tên là `kit_vna_can.dbc` (có sẵn trong thư mục dự án) vào phần mềm.
2. Lúc này toàn bộ các ID kỹ thuật sẽ tự động biến thành tiếng Việt (Ví dụ: *Thoi_Gian_Thuc_RTC, Muc_Tieu...*).
3. **Cách đổi số lượng Mục Tiêu:** Tìm đến khung CAN TX (ID `0x201`), nhập một số thập phân bình thường (ví dụ: `100`) rồi bấm nút Gửi (Send). Mạch sẽ nhớ con số 100 này làm mục tiêu.
4. **Cách chỉnh giờ khi đồng hồ sai:** Tìm đến khung CAN TX (ID `0x203`), nhập đúng thứ tự: *Ngày(1->31), Tháng(1->12), NăM(0->99), Thứ(1->7)*. Khung giờ nhập *Giờ(0->23), Phút(0->59), Giây(0->59)* (Dưới dạng số thập phân thông thường), rồi bấm Gửi (Send). Mạch sẽ nhận giờ lập tức, hết nhấp nháy chữ `----`.
5. **Cách đặt số đếm bất kỳ:** Tìm khung CAN TX (ID `0x202`), nhập một số bất kỳ (ví dụ: `50`), bấm Gửi, mạch sẽ lập tức nhảy số đếm về 50.

6. **Thay đổi tính năng của Thẻ RFID (Nạp lệnh mới vào thẻ):**
   - Mở khung CAN TX **`ID 0x204 (Cai_Dat_Tinh_Nang_RFID)`**.
   - Tại cột **Value**, nhập 1 trong các số sau tùy theo chức năng muốn cài:
     - `1`: Reset đếm (`RESET_DEM`)
     - `2`: Bật Relay (`BAT_RELAY`)
     - `3`: Đảo Relay (`DAO_RELAY`)
     - `4`: Phát nhạc (`PHAT_NHAC`)
   - Bấm Gửi. Mạch sẽ kêu **2 bíp ngắn dồn dập (50ms)** báo hiệu đang ở chế độ chờ ghi thẻ. Lúc này bạn quẹt thẻ từ vào mâm đọc, mạch sẽ nạp lệnh mới vào thẻ. (Quẹt xong sẽ có tiếng bíp dài báo thành công).
   *(Lưu ý : Để có thể thay đổi tính năng thẻ thì yêu cầu mật khẩu (key A) của thẻ phải là **0x11 0x22 0x33 0x44 0x55 0x66**)
7. **Đổi mật khẩu / Xóa trắng thẻ RFID:**
   - Mở khung CAN TX **`ID 0x205 (Cai_Dat_Mat_Khau_The)`**.
   - Cột **Lenh_Dieu_Khien (Byte 0):** Nhập `0` nếu muốn Xóa tính năng của thẻ , hoặc nhập `1` nếu muốn Đổi mật khẩu mới.
   - **Cách điền Mật khẩu mới:**
     - Nếu bạn muốn nhập mật khẩu bằng hệ **Hexa**, hãy click đúp chuột trực tiếp vào khung **Data** (chữ số ngoài cùng) và sửa trực tiếp chuỗi Hex (Ví dụ: `01 0F FF FF FF FF FF` rồi ấn Enter). 
     - Nếu nhập từng cột Value, hãy quy đổi ra số Thập phân (Ví dụ `0xFF` thì gõ là `255`, `0x0F` thì gõ là `15`). Cangaroo không hiểu chữ "F" nếu gõ chay.
   - Bấm Gửi. Mạch kêu **2 bíp dồn dập** chờ thẻ. Quẹt thẻ để hoàn tất quá trình đổi mật khẩu.
    *(Lưu ý : Để có thể đổi mật khẩu mới cho thẻ cần biết mật khẩu hiện tại của thẻ)*

8. **Cấp Chìa khóa Tạm thời :**
   - Theo mặc định, mạch sẽ tự động truy cập thẻ bằng mật khẩu (Key A) là `11 22 33 44 55 66`. Tuy nhiên, nếu thẻ có mật khẩu lạ, sẽ không thể thao tác Ghi lệnh (Bước 6) hay Xóa/Đổi mật khẩu (Bước 7) được nữa, mạch sẽ kêu 2 tiếng bíp dài báo sai mật khẩu.
   - Để khắc phục, cần mở khung CAN TX **`ID 0x206 (Cai_Dat_Mat_Khau_Tam_Thoi)`**.
   - Tại phần Data, nhập 6 byte mật khẩu hiện tại của thẻ (ví dụ `AA BB CC DD EE FF`), rồi bấm Gửi.
   - Mạch sẽ kêu **2 bíp ngắn dồn dập**. Ngay lúc này, mạch đã lưu chìa khóa tạm thời vào bộ nhớ RAM. Có thể tiến hành gửi lệnh `0x204` (Đổi tính năng) hoặc `0x205` (Xóa/Đổi mật khẩu) như bình thường. Chìa khóa tạm này sẽ được sử dụng cho đúng 1 lần quẹt thẻ kế tiếp.

9. **Quan sát Dữ liệu Mạch gửi lên PC (Monitor):**
   - Chuyển sang bảng **CAN RX** (Hoặc tab Monitor) để xem mạch báo cáo trạng thái theo thời gian thực:
   - **`ID 0x101`**: Thông báo có thẻ vừa quẹt. Xem được mã ID của thẻ (5 byte đầu) và trạng thái đọc thẻ (Byte cuối: `0` là Đọc thành công, `1-3` là có lỗi).
   - **`ID 0x102`**: Gửi liên tục thông số Số đếm hiện tại và Mục tiêu. (Bạn có thể thấy số đếm nhảy lên ngay lập tức trên máy tính khi che tay qua cảm biến).
   - **`ID 0x103`**: Gửi liên tục Thời gian thực (giờ,phút,giây,ngày,tháng,năm,thứ) từ vi điều khiển để bạn đồng bộ.
   - **`ID 0x104 & 0x105`**: Chứa chuỗi ký tự dữ liệu thô mạch đọc được từ Thẻ RFID khi bạn quẹt thẻ.

---

## 4. HƯỚNG DẪN DÀNH CHO LẬP TRÌNH VIÊN

Mã nguồn được tổ chức theo cấu trúc Hướng đối tượng (OOP) C++. Các class ngoại vi được thiết kế theo mô hình Non-blocking, không sử dụng hàm trễ `HAL_Delay`, đảm bảo mạch có thể đa nhiệm mượt mà.

**Lưu ý:** Bộ thư viện này viết trên nền tảng STM32 HAL (cụ thể là chip **STM32F1**). Để mang sang dòng chip khác (STM32F4, G0...), bạn chỉ cần vào các file `.h` và đổi `#include "stm32f1xx_hal.h"` thành thư viện HAL tương ứng.

---

### 4.1. Thư viện `Buzzer`
**Hướng dẫn Setup (Cài đặt):**
- **Include:** `#include "Buzzer.h"`
- **Vị trí khai báo:** Khai báo đối tượng ở Global (đầu file `main.cpp`).
- **Khởi tạo (`init`):** Gọi hàm `.init()` trong vùng `USER CODE BEGIN 2` để nạp cấu hình chân.
- **Duy trì thời gian:** Bạn BẮT BUỘC phải đặt hàm tĩnh `buzzer.update()` vào một ngắt Timer định kỳ 1ms (thường là hàm `SysTick_Handler` trong file `stm32f1xx_it.c`). Nếu thiếu, hàm bíp theo hàng đợi sẽ bị liệt.

**Ví dụ Code:**
```cpp
/* --- Trong file main.cpp --- */
#include "Buzzer.h"

// 1. Khai báo Global (Tham số: Port, Pin)
Buzzer buzzer(GPIOC, GPIO_PIN_15); 

int main(void) {
    // ... code HAL_Init ...
    
    // 2. Khởi tạo
    buzzer.init();
    
    // 3. Sử dụng lệnh phát âm thanh 
    buzzer.beep(3, 100, 100);       // Kêu 3 lần, nghỉ 100ms (nối tiếp vào hàng đợi)
    buzzer.beep(1, 100, 0, true);   // Kêu ngay lập tức (Xóa hàng đợi cũ)
    
    while(1) { /* ... */ }
}

// 4. Duy trì thời gian (Ghi đè hàm của HAL ngay trong main.cpp)
// Trong project này, thay vì viết vào stm32f1xx_it.c, chúng ta ghi đè trực tiếp hàm HAL_IncTick:
void HAL_IncTick(void) {
    uwTick += uwTickFreq;
    buzzer.update();  // Bộ đếm thời gian cho còi
}
```

### 4.2. Thư viện `Mp3` (VS1011e)
**Hướng dẫn Setup (Cài đặt):**
- **Include:** `#include "Mp3.h"`
- **Phụ thuộc:** Yêu cầu thư viện `FatFs` phải được kích hoạt trước (chạy lệnh `f_mount`).
- **Duy trì luồng:** Hàm `mp3.play()` chỉ dò tìm mở file, không tự phát nhạc ra loa. Bắt buộc đặt `mp3.process()` vào vòng lặp `while(1)` để duy trì tác vụ đọc byte thẻ nhớ và bơm sang SPI.

**Lưu ý kỹ thuật:**
- **Về mạch Amply ngoài (Dành riêng cho Kit hiện tại):** Bản thân thư viện `Mp3` (VS1011e) xuất trực tiếp ra tín hiệu âm thanh. Tuy nhiên, phần cứng của Kit này có gắn thêm một IC khuếch đại (MIX2018A) nối với loa, và được điều khiển (Mute) qua chân `PA4`. Do đó ở ví dụ dưới, bạn phải kéo chân `PA4` xuống LOW thì loa mới kêu. Nếu bạn ứng dụng thư viện này sang đồ án/mạch điện khác không có mạch Amply này, bạn hoàn toàn có thể bỏ qua bước điều khiển chân `PA4`!
- Hàm `saveState()` và `resumeState()` hỗ trợ chức năng ghi nhớ vị trí file âm thanh khi bạn bị tuột/cắm nóng thẻ nhớ.

**Ví dụ Code:**
```cpp
#include "Mp3.h"

// 1. Khai báo Global (SPI_Handle, XRESET, DREQ, XDCS, XCS)
Mp3 mp3(&hspi2, 
        GPIOA, GPIO_PIN_0,  // XRESET
        GPIOA, GPIO_PIN_1,  // DREQ
        GPIOB, GPIO_PIN_12, // XDCS
        GPIOC, GPIO_PIN_3   // XCS
);

int main(void) {
    // ...
    mp3.init();
    
    // Bật Amply (PA4) trước khi chơi nhạc
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
    
    // 2. Phải Mount thẻ nhớ thành công mới mở được file
    FATFS fs;
    if (f_mount(&fs, "", 1) == FR_OK) {
        mp3.play("DAT.MP3"); 
    }

    // 3. Vòng lặp chính
    while (1) {
        mp3.process(); // Hàm nòng cốt để loa phát ra tiếng liên tục
    }
}
```

### 4.3. Thư viện `SdCard`
**Hướng dẫn Setup (Cài đặt):**
- **Include:** Không cần include vào `main.cpp` nếu bạn chỉ thao tác đọc/ghi file. 
- **Cách hoạt động:** Lớp `SdCard` chỉ phụ trách gửi lệnh vật lý (CMD) qua SPI. Các hàm của nó đã được nhúng thẳng vào file `diskio.c` của thư viện `FatFs`. 
- **Sử dụng:** Bạn thao tác 100% thông qua các lệnh chuẩn của FatFs (như `f_mount`, `f_open`). Thư viện FatFs sẽ tự động gọi ngược lại thư viện `SdCard` để điều khiển chip.

**Ví dụ Code:**
```cpp
#include "ff.h" // Thư viện hệ thống file FatFs
#include "SdCard.h"

// 1. Khai báo (SPI_Hardware, CS_Port, CS_Pin) 
// Khai báo bằng thanh ghi SPI1, không dùng cấu trúc &hspi1 của HAL
SdCard sdCard(SPI1, GPIOC, GPIO_PIN_8);

int main(void) {
    // ...
    sdCard.init(); // Cấu hình chân IO CS
    
    FATFS fs;
    // 2. Hàm f_mount sẽ tự động gọi sdCard.initialize() ẩn bên dưới để khởi động SPI
    if (f_mount(&fs, "", 1) == FR_OK) {
        // SD Card đã sẵn sàng
    }
    
    while(1) {}
}
```

### 4.4. Thư viện `Rfid` (MFRC522)
**Hướng dẫn Setup (Cài đặt):**
- **Include:** `#include "Rfid.h"`
- **Giao thức:** Yêu cầu bus SPI ổn định. Nếu phát hiện mạch bị mất module hoặc cắm rút nóng thẻ nhớ chung bus SPI, bắt buộc phải gọi lại `rfid.init()` để thiết lập lại dao động.

**Ví dụ Code quy trình chuẩn:**
```cpp
#include "Rfid.h"

// 1. Khai báo Global (SPI_Hardware, CS_Port, RST_Port)
Rfid rfid(SPI3, 
          GPIOA, GPIO_PIN_15, // CS (SDA)
          GPIOC, GPIO_PIN_14  // RST
);

int main(void) {
    // ...
    rfid.init(); // Reset IC và bật anten
    
    while (1) {
        // 2. Vòng lặp Quét và Đọc thẻ
        if (rfid.checkCard()) {
            uint8_t uid[5];
            
            // Lấy mã UID của thẻ
            if (rfid.readCardSerial(uid)) {
                rfid.selectTag(uid); // Khóa thẻ mục tiêu
                
                // Xác thực bộ mã (Key) để truy cập Block 1
                uint8_t key[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
                if (rfid.authenticate(MFRC522_CMD_MF_AUTH_KEY_A, 1, key, uid) == MI_OK) {
                    
                    // Thao tác đọc dữ liệu (16 bytes)
                    uint8_t data[16]; 
                    uint8_t size = 16;
                    rfid.MIFARE_Read(1, data, &size);
                }
                
                // Cực kỳ quan trọng: Ép thẻ ngủ sau khi làm xong
                rfid.PICC_HaltA(); 
            }
        }
    }
}
```

### 4.5. Thư viện `Rtc` (DS1307)
**Hướng dẫn Setup (Cài đặt):**
- **Include:** `#include "Rtc.h"`
- **Cấu hình I2C:** Thư viện giao tiếp qua bus I2C. Bạn cần truyền chuẩn xác thanh ghi phần cứng (`I2C1` hoặc `I2C2`) cùng với các chân SCL và SDA tương ứng vào constructor để thư viện tự động khởi tạo.
- **Báo lỗi mất thời gian:** Hàm `isTimeLost()` quét bit Clock Halt (CH). Nếu pin CMOS cạn nguồn, RTC sẽ dừng đếm, hàm này trả về `true` để bạn cảnh báo lên UI.

**Ví dụ Code:**
```cpp
#include "Rtc.h"

// 1. Khai báo (I2C_Hardware, SCL_Port, SCL_Pin, SDA_Port, SDA_Pin)
Rtc rtc(I2C1, GPIOB, GPIO_PIN_6, GPIOB, GPIO_PIN_7);

int main(void) {
    // ...
    rtc.init();
    
    // Báo lỗi: Thời gian sai, cần đồng bộ lại
    if (rtc.isTimeLost()) {
        // Xử lý báo lỗi
    }
    
    while(1) {
        uint8_t h, m, s, dow, date, month, year;
        
        // Cập nhật dữ liệu ra biến
        rtc.getTime(h, m, s);
        rtc.getDate(dow, date, month, year);
        
        // Set dữ liệu mới (ví dụ lúc 12:30:00)
        // rtc.setTime(12, 30, 0); 
    }
}
```

### 4.6. Thư viện `Led7Segment` (CD4094)
**Hướng dẫn Setup (Cài đặt):**
- **Include:** `#include "Led7Segment.h"`
- **Giao tiếp:** Chuẩn 3 dây IC dịch `CD4094` (Data, Clock, Strobe).
- **Luồng dữ liệu:** Hàm `setValue()` chỉ nạp dữ liệu tạm vào RAM. Bắt buộc gọi hàm `update()` để chip Dịch và Strobe chốt xuất dữ liệu ra chân LED.

**Ví dụ Code:**
```cpp
#include "Led7Segment.h"

// 1. Khai báo Global (DATA, CLOCK, STROBE)
Led7Segment dp(
    GPIOB, GPIO_PIN_5, // DATA
    GPIOB, GPIO_PIN_4, // CLOCK
    GPIOB, GPIO_PIN_3  // STROBE
);

int main(void) {
    // ...
    dp.init();
    
    while(1) {
        dp.setValue(1234); // Nạp 1234
        dp.setColon(true); // Bật dấu (:) 
        
        // Gọi lệnh kết xuất ra LED. 
        // Truyền false để xóa số 0 vô nghĩa (chỉ hiện ' 123' thay vì '0123')
        dp.update(false); 
    }
}
```

### 4.7. Thư viện `CanBus`
**Hướng dẫn Setup (Cài đặt):**
- **Include:** `#include "CanBus.h"` 
- **Quy tắc ghép Byte :** Giao thức CAN gửi dữ liệu thành các đoạn cắt nhỏ 8-bit (1 byte). Phần mềm trên PC (ví dụ Cangaroo) sử dụng kiến trúc Little-Endian, nghĩa là khi một số lớn (như số 16-bit) bị cắt đôi gửi đi, **Byte thấp (Low Byte)** sẽ được gửi trước nằm ở `rxData[0]`, và **Byte cao (High Byte)** gửi sau nằm ở `rxData[1]`.
  *Cách khôi phục:* Để ghép chúng lại thành số gốc, cần lấy byte cao dịch sang trái 8 bit (`rxData[1] << 8`) rồi dùng phép toán OR (`|`) gộp với byte thấp (`rxData[0]`). Cú pháp: `uint16_t value = rxData[0] | (rxData[1] << 8);`

**Ví dụ Code:**
```cpp
#include "CanBus.h"

CanBus myCan(&hcan); // Dùng HAL handle có sẵn

int main(void) {
    // ...
    myCan.init(); // Áp dụng bộ lọc cơ bản nhất (Chấp nhận tất cả ID)
    
    // Gửi đi (ID = 0x123, Dài = 2 byte)
    uint8_t txData[2] = {0x01, 0x02};
    myCan.sendFrame(0x123, txData, 2);
    
    while (1) {
        uint32_t rxId;
        uint8_t rxData[8], rxLen;
        
        // Vòng lặp lấy Data không trễ (Non-blocking)
        if (myCan.readFrame(&rxId, rxData, &rxLen)) {
            // Ghép 2 byte đầu tiên thành biến 16-bit
            uint16_t value = rxData[0] | (rxData[1] << 8);
        }
    }
}
```

### 4.8. Các class `Sensor` và `Relay`
**Hướng dẫn Setup (Cài đặt):**
- **Relay:** `Relay.h` dùng để đảo trạng thái Output cơ bản.
- **Sensor:** `Sensor.h` BẮT BUỘC bạn cấu hình phần cứng chân cảm biến ở chế độ **Ngắt ngoài EXTI**.
- Logic ngắt (hàm `EXTI15_10_IRQHandler`) đã được lập trình sẵn bên trong file `Sensor.cpp`. Bên trong thư viện đã tích hợp code triệt tiêu nhiễu cơ học (Debounce = 20ms).
- **Chế độ Diagnostic Mode (Đo đạc nhiễu):** Kích hoạt bằng `Sensor::enableDiagnosticMode(true)`. EXTI sẽ được cấu hình để bắt cả cạnh lên và cạnh xuống. Giúp vi điều khiển đo được thời gian (mili-giây) sản phẩm che qua cảm biến và thời gian trống giữa 2 sản phẩm. Tính năng này cực kỳ hữu ích để gỡ lỗi khi đếm các sản phẩm trong suốt hoặc hình dạng bất thường gây ra xung giả.
**Ví dụ Code:**
```cpp
/* --- Trong file main.cpp --- */
#include "Relay.h"
#include "Sensor.h"

Relay relay(GPIOA, GPIO_PIN_1);
Sensor sensor(GPIOC, GPIO_PIN_13);

int main(void) {
    // ...
    relay.init();
    sensor.init();
    
    // (Tùy chọn) Kích hoạt chế độ đo đạc độ rộng xung cảm biến 
    Sensor::enableDiagnosticMode(true);
    
    while(1) {
        relay.toggle();
        HAL_Delay(1000);
    }
}
```
