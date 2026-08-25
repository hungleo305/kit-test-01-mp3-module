# Danh Sách Công Việc & Kế Hoạch Cập Nhật Firmware 

Tài liệu này theo dõi tiến độ các hạng mục cần nâng cấp/sửa lỗi cho Firmware của dự án.

---

## 1. Cải thiện độ ổn định Hệ thống 

### Task 1: Xử lý lỗi quẹt đè thẻ RFID làm gián đoạn âm thanh
- **Công việc:** Cấu hình bộ đệm vòng (Circular Queue) cho luồng âm thanh để các lệnh phát nhạc được xếp hàng chờ phát lần lượt thay vì đè lên nhau.
- **Kết quả kỳ vọng:** 
  1. Quẹt 1 thẻ và ngay lập tức quẹt thêm thẻ thứ 2. Kết quả mong đợi: Âm thanh thẻ 1 phát xong, ngay lập tức phát tiếp âm thanh thẻ 2.
  2. Quẹt 6 thẻ liên tiếp với tốc độ nhanh. Kết quả mong đợi: Phát lần lượt 5 âm thanh (do sức chứa hàng đợi là 5) và không bị treo mạch (HardFault).

### Task 2: Bảo toàn giá trị bộ đếm sản phẩm khi mất nguồn
- **Công việc:** Lưu trữ biến đếm sản phẩm liên tục vào thanh ghi dự phòng (BKP Register) được nuôi bằng pin CMOS thay vì lưu trên RAM thông thường.
- **Kết quả kỳ vọng:**
  1. Cho cảm biến đếm. Rút điện mạch đột ngột. Cắm điện lại. Kết quả mong đợi: Màn hình hiển thị ngay lập tức giá trị đã đếm tới.
  2. Bắn lệnh set số đếm qua mạng CAN thành 5000. Rút điện và cắm lại. Kết quả mong đợi: Giữ nguyên 5000.

### Task 3: Tăng độ nhạy ngắt cảm biến & Máy chẩn đoán quang học
- **Công việc:** Giảm thời gian Debounce của ngắt EXTI xuống 20ms, cấu hình bắt cả cạnh lên và cạnh xuống. Xây dựng bộ công cụ đo đạc thời gian bị che khuất và khoảng hở giữa các lần che.
- **Kết quả kỳ vọng:**
  1. Dùng tay (hoặc vật cản) lướt qua cảm biến nhanh 2 - 3 lần liên tục. Kết quả mong đợi: Còi bíp đủ số lần, bộ đếm nhảy đủ số lượng.
  2. Bật chế độ Diagnostic Mode trong main.cpp, che tay vào cảm biến rồi rút ra. Kết quả mong đợi: Cangaroo nhận được gói tin CAN 0x124 dài 8 Byte. 4 Byte đầu là thời gian che (Block Time), 4 Byte sau là khoảng hở (Gap Time) tính bằng mili-giây.

---

## 2. Nâng cấp module thẻ nhớ SD & Audio (SD Card & MP3)

### Task 4: Tối ưu cơ chế nhận diện và kết nối thẻ SD
- **Công việc:** Sử dụng ngắt cứng Detect (PB0) thay vì vòng lặp Polling để nhận diện thẻ. Nâng cấp cấu hình thư viện để tương thích với các thẻ dung lượng cao SDHC/SDXC (16GB, 64GB).
- **Kết quả kỳ vọng:**
  1. Đang chạy bình thường, rút thẻ SD ra. Kết quả mong đợi: Còi báo hiệu thẻ lỗi 1 tiếng bíp ngay lập tức (không cần đợi vài giây).
  2. Cắm thẻ SD tốt vào lại. Kết quả mong đợi: Mạch Mount thành công, bíp 2 tiếng và sẵn sàng.
  3. Cắm thẻ SD hỏng/chưa format đúng FAT32 vào. Kết quả mong đợi: Mạch báo lỗi (bíp 3 hoặc 5 tiếng) rồi nghỉ, CPU rảnh rỗi , quẹt thẻ RFID vẫn mượt mà không bị đơ.

### Task 5: Đo tốc độ nhận diện thẻ SD & Triệt tiêu âm thanh "bụp" khởi động
- **Công việc:** Dùng SystemTimer::getTick() đo thời gian khởi tạo thẻ và truyền qua CAN. Chặn tiếng bụp bằng cách cấu hình chân MUTE của Amply (PA4) đóng ngay khi khởi động và trong lúc reset thẻ.
- **Kết quả kỳ vọng:**
  1. Cấp nguồn hoặc bấm Reset hệ thống. Kết quả mong đợi: Loa hoàn toàn im lặng, không có tiếng bụp.
  2. Rút thẻ SD và cắm lại nóng. Kết quả mong đợi: Đèn LED hiển thị thời gian kết nối (ms), loa im lặng không xẹt nhiễu, sau đó báo 2 bíp và phát nhạc bình thường. Dữ liệu trên CAN báo đúng BCD frame 0x125.

---

### Task 6: Lưu và khôi phục trạng thái phát nhạc khi nóng-rút thẻ (Hot-plug Resume)
- **Công việc:** Lưu lại con trỏ thời gian của file nhạc hiện tại vào RAM khi thẻ bị rút. Tự động phục hồi phát tiếp khi cắm thẻ lại. Bổ sung ghi thẻ RFID PHAT_NHAC thông qua CAN.
- **Kết quả kỳ vọng:**
  1. Cho chạy file nhạc dài MUSIC.MP3 bằng thẻ RFID. Đột ngột rút thẻ nhớ.
  2. Cắm thẻ nhớ lại. Kết quả mong đợi: Bản nhạc tự động nhảy tới đúng mili-giây bị ngắt và hát nối tiếp mượt mà.

---

## 3. Cải thiện mạng CAN

### Task 7: Tối ưu mạng CAN và Cảnh báo lỗi
- **Công việc:**
  - Tối ưu hóa băng thông (chỉ gửi frame định kỳ/sự kiện).
  - Kích hoạt chế độ tự phục hồi sau khi mất kết nối.
  - Viết file kit_vna_can.dbc để hiển thị tiếng Việt và trạng thái trên Cangaroo.
  - Bổ sung chớp màn hình ---- và báo còi khi đứt mạng.
- **Kết quả kỳ vọng:**
  1. Mất kết nối CAN. Kết quả mong đợi: Còi bíp dài 1 lần, màn hình chớp ---- trong 3 giây.
  2. Cắm lại cáp CAN. Kết quả mong đợi: Mạch hoạt động bình thường
  3. Mở Cangaroo bằng file .dbc. Kết quả mong đợi: Đọc được các bản tin cảnh báo bằng tiếng Việt.

---

## 4. Quản lý tài nguyên

### Task 8: Đánh giá và tối ưu tài nguyên Chip 
- **Công việc:** Đo đạc mức sử dụng CPU (CPU Load), mức độ sử dụng RAM và Stack để đảm bảo mạch không bao giờ bị quá tải hoặc tràn bộ nhớ.
- **Kết quả kỳ vọng:**
  1. Gắn các bộ đếm vào Main Loop và SystemTimer để tính toán % thời gian CPU rảnh rỗi. Kết quả mong đợi: Idle time > 60% khi hoạt động full tải (vừa đọc thẻ, vừa quẹt RFID, vừa chạy MP3).
