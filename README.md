# TÀI LIỆU HƯỚNG DẪN SỬ DỤNG VÀ PHÁT TRIỂN FIRMWARE - KIT VNA

Dự án mạch Kit điều khiển đa năng giao tiếp CAN Bus, tích hợp đọc thẻ RFID, thẻ nhớ SD (phát nhạc MP3), cảm biến đếm sản phẩm, và LED 7 đoạn.

---

## 1. CẤU TRÚC THƯ MỤC DỰ ÁN

`	Text
kit-test-01-mp3-module/
├── Core/
│   ├── Inc/                # Header thư viện (.h)
│   │   ├── Buzzer.h        # Còi chip
│   │   ├── CanBus.h        # Mạng CAN
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
├── kit_vna_can.dbc         # Database ánh xạ dữ liệu CAN
├── danh_sach_cong_viec.md  # Danh sách công việc
└── README.md               # Tài liệu này
`
---

## 2. HƯỚNG DẪN SỬ DỤNG DÀNH CHO NGƯỜI DÙNG (KHÔNG CẦN BIẾT LẬP TRÌNH)

Phần này hướng dẫn chi tiết cách vận hành mạch từ lúc cắm điện đến lúc sử dụng các tính năng. Bạn chỉ cần làm theo các bước dưới đây.

### 2.1. Cắm điện và Khởi động
- **Cắm điện:** Ngay khi vừa cấp nguồn, mạch sẽ khởi động hệ thống và kêu **2 tiếng Bíp** để báo hiệu đã có điện.
- **Kết nối Thẻ Nhớ:** Sau tiếng bíp khởi động, mạch sẽ tự động đọc thẻ nhớ (SD Card). Nếu đọc thành công, mạch sẽ kêu thêm **2 tiếng Bíp** nữa. (Tổng cộng là 4 tiếng bíp nếu cắm sẵn thẻ nhớ tốt).
- **Đèn LED Main (Trái tim của mạch):** Đèn LED nhỏ trên thân mạch (LED Main) sẽ chớp nháy liên tục rất nhanh (chu kỳ 0.1 giây sáng / 0.1 giây tắt - tức là chớp 5 lần/giây). Đèn này nháy đều đặn chứng tỏ "trái tim" vi xử lý đang chạy cực kỳ trơn tru và không bị treo.
- **Màn hình LED 7 đoạn:** Sẽ bắt đầu tự động luân phiên hiển thị giữa 2 thông tin (mỗi thông tin hiển thị 5 giây):
  1. **Số lượng sản phẩm đã đếm được:** Dấu chấm ở giữa màn hình sẽ **TẮT**.
  2. **Giờ hiện tại (Giờ:Phút):** Dấu chấm ở giữa màn hình sẽ **NHẤP NHÁY** theo từng giây.

### 2.2. Cách đọc Cảnh báo bằng Còi và Màn hình
Mạch có hệ thống tự động cảnh báo lỗi thông minh:
- **Lỗi đứt dây mạng CAN (hoặc chưa cắm dây máy tính):** Còi kêu **10 tiếng bíp nhanh**. Sau đó, ở màn hình đếm số, dấu chấm sẽ nhấp nháy cực kỳ nhanh (5 lần/giây) để nhắc nhở không gửi dược dữ liệu qua canbus
- **Lỗi mất thời gian thực :** Lúc mới bật máy sẽ im lặng hoàn toàn. Tới lúc màn hình chuyển sang xem giờ, thay vì hiện giờ, màn hình sẽ hiện chữ ---- nhấp nháy liên tục (1 lần/giây) mãi mãi. Cần nạp lại thời gian qua canbus.
- **Lỗi Thẻ Nhớ (Thẻ hỏng, chưa cắm, hoặc sai định dạng):** Còi sẽ kêu 1 tiếng bíp dài báo lỗi. Mạch vẫn sẽ đếm sản phẩm và quẹt thẻ bình thường nhưng sẽ không phát ra bài nhạc nào cả.

### 2.3. Hướng dẫn sử dụng Thẻ Nhớ MP3 (Thẻ SD)
- **Chuẩn bị thẻ:** Rút thẻ ra cắm vào máy tính, Format thẻ với định dạng **FAT32**. Sau đó copy các bài nhạc (đuôi file là .mp3) trực tiếp vào thẻ, không bỏ vào thư mục nào cả. 
- **Rút/Cắm thẻ khi đang chạy (Hot-plug):** Nếu đang phát nhạc mà bạn vô tình rút thẻ ra -> Mạch kêu 1 bíp, tự động ngắt amply để loa không bị nổ lụp bụp, đồng thời ghi nhớ vị trí bài hát đang nghe. Khi cắm thẻ lại -> Mạch kêu 2 bíp và **tự động hát tiếp** đúng ngay đoạn nhạc vừa bị ngắt!

### 2.4. Hướng dẫn đếm sản phẩm bằng Cảm biến
- **Cách đếm:** Khi có sản phẩm chạy ngang qua mắt cảm biến, còi kêu 1 tiếng **Bíp** và số trên màn hình tăng lên 1. 
- **Tính năng lấn át:** Nếu màn hình đang ở chế độ "Xem giờ" mà có hàng chạy ngang qua, màn hình sẽ **lập tức nhảy về chế độ Số Đếm** để bạn dễ dàng theo dõi.
- **Đạt mục tiêu:** Khi số lượng sản phẩm đếm được bằng với con số "Mục tiêu" (có thể cài đặt qua máy tính), loa sẽ tự động phát to bài nhạc báo động (tên file là DAT.MP3).

### 2.5. Hướng dẫn Quẹt thẻ (RFID)
Mạch đi kèm bộ thẻ từ . Quẹt các thẻ này vào anten (miếng vuông in chữ RC522) để ra lệnh cho mạch làm việc:
- Quẹt thẻ **RESET**: Mạch tự động xóa số đếm sản phẩm về 0 và loa phát bài RESET.MP3.
- Quẹt thẻ **BAT**: Bật công tắc điện (Relay) trên mạch, loa phát bài BAT.MP3.
- Quẹt thẻ **DAO**: Đổi ngược trạng thái công tắc (Đang bật thành tắt, đang tắt thành bật), loa phát bài DAO.MP3.
- Quẹt thẻ **PHAT_NHAC**: Ra lệnh cho loa phát một bài nhạc giải trí tên là MUSIC.MP3.
*(Mẹo: Có thể quẹt liên tục nhiều thẻ khác nhau, mạch sẽ tự động xếp hàng và xử lý mượt mà, không bị treo hay tiếng bị vỡ).*

### 2.6. Hướng dẫn dùng phần mềm máy tính (CAN Bus)
Bạn cắm mạch vào máy tính qua bộ chuyển đổi CAN. Mở phần mềm **Cangaroo** trên máy tính:
1. Load file có sẵn tên là kit_vna_can.dbc (có sẵn trong thư mục dự án) vào phần mềm.
2. Lúc này toàn bộ các con số kỹ thuật sẽ tự động biến thành tiếng Việt (Ví dụ: *Dem_San_Pham, Thoi_Gian_Che, Nam_Moi...*).
3. **Cách đổi số lượng Mục Tiêu:** Tìm đến phần "Mục tiêu", gõ một con số bạn muốn (ví dụ: 100) rồi bấm nút Gửi (Send). Mạch sẽ nhớ con số 100 này làm mục tiêu.
4. **Cách chỉnh giờ khi đồng hồ sai:** Tìm đến khung x203 (Chỉnh thời gian), nhập con số ngày/tháng/năm và giờ/phút/giây hiện tại vào, rồi bấm Gửi (Send). Mạch sẽ nhận giờ lập tức, màn hình sẽ hết nhấp nháy chữ ---- và hiển thị giờ bình thường.
5. **Cách đặt số đếm bất kỳ:** Có thể nhập một số bất kỳ (ví dụ: 50) vào khung "Set giá trị đếm", bấm Gửi, mạch sẽ lập tức nhảy số đếm về 50.

---

## 3. HƯỚNG DẪN SỬ DỤNG THƯ VIỆN (LẬP TRÌNH VIÊN)

