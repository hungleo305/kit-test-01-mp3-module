# DEADLINE : THỨ 4,26-08-2026
# Danh Sách Công Việc & Kế Hoạch Cập Nhật Firmware 

Tài liệu này theo dõi tiến độ các hạng mục cần nâng cấp/sửa lỗi cho Firmware của dự án.

---

## 1. Cải thiện độ ổn định Hệ thống 

### Task 1: Xử lý lỗi quẹt đè thẻ RFID làm gián đoạn âm thanh
- **Trạng thái:** Đã hoàn thành
- **Mục tiêu:** Khi người dùng quẹt nhiều thẻ liên tiếp, các luồng âm thanh phải xếp hàng và phát nối tiếp nhau thay vì cắt ngang nhau.
- **Kịch bản kiểm thử:**
  1. Quẹt 1 thẻ và ngay lập tức quẹt thêm thẻ thứ 2. Kết quả mong đợi: Âm thanh thẻ 1 phát xong, ngay lập tức phát tiếp âm thanh thẻ 2.
  2. Quẹt 6 thẻ liên tiếp với tốc độ nhanh. Kết quả mong đợi: Phát lần lượt 5 âm thanh (do sức chứa hàng đợi là 5) và không bị treo mạch (HardFault).

### Task 2: Bảo toàn giá trị bộ đếm sản phẩm khi mất nguồn
- **Trạng thái:** Đã hoàn thành
- **Mục tiêu:** Lưu trữ biến đếm sản phẩm liên tục vào phân vùng an toàn, không phụ thuộc vào chu kỳ sống của RAM.
- **Kịch bản kiểm thử :**
  1. Chạy băng chuyền cho đếm đến 150. Rút điện mạch đột ngột. Cắm điện lại. Kết quả mong đợi: Màn hình hiển thị ngay lập tức số 150.
  2. Bắn lệnh set số đếm qua mạng CAN thành 5000. Rút điện và cắm lại. Kết quả mong đợi: Giữ nguyên 5000.

### Task 3: Tăng độ nhạy ngắt cảm biến & Máy chẩn đoán quang học
- **Trạng thái:** Đã hoàn thành
- **Mục tiêu:** Cảm biến phải bắt được vật thể lướt qua với tốc độ cực cao. Xây dựng công cụ đo đạc thời gian thực tế cảm biến bị che khuất và khoảng hở giữa các lần che.
- **Kịch bản kiểm thử :**
  1. Dùng tay (hoặc vật cản) lướt qua cảm biến cực nhanh 2 - 3 lần liên tục. Kết quả mong đợi: Còi bíp đủ số lần, bộ đếm nhảy đủ số lượng.
  2. Bật chế độ `Diagnostic Mode` trong `main.cpp`, che tay vào cảm biến rồi rút ra. Kết quả mong đợi: Cangaroo nhận được gói tin CAN `0x124` dài 8 Byte (mã BCD hệ thập phân dễ đọc). 4 Byte đầu là thời gian che (Block Time), 4 Byte sau là khoảng hở (Gap Time) tính bằng mili-giây.

---

## 2. Nâng cấp module thẻ nhớ SD & Audio (SD Card & MP3)

### Task 4: Tối ưu cơ chế nhận diện và kết nối thẻ SD
- **Trạng thái:** Đã hoàn thành
- **Mục tiêu:** Tự động phát hiện thẻ bị rút/cắm vào ngay lập tức qua chân cứng Detect (PB0) thay vì Polling. Chống nhiễu chéo từ RFID, ngăn chặn vòng lặp khởi tạo vô tận (treo hệ thống) khi cắm thẻ hỏng. Nâng cấp thư viện hỗ trợ tương thích hoàn hảo thẻ dung lượng cao SDHC/SDXC (16GB, 64GB).
- **Kịch bản kiểm thử :**
  1. Đang chạy bình thường, rút thẻ SD ra. Kết quả mong đợi: Còi báo hiệu thẻ lỗi 1 tiếng bíp ngay lập tức (không cần đợi vài giây).
  2. Cắm thẻ SD tốt vào lại. Kết quả mong đợi: Mạch Mount thành công, bíp 2 tiếng và sẵn sàng.
  3. Cắm thẻ SD hỏng/chưa format đúng FAT32 vào. Kết quả mong đợi: Mạch báo lỗi (bíp 3 hoặc 5 tiếng) rồi nghỉ, CPU rảnh rỗi tuyệt đối, quẹt thẻ RFID vẫn mượt mà không bị đơ.

### Task 5: Đo tốc độ nhận diện thẻ SD & Triệt tiêu âm thanh "bụp" khởi động
- **Trạng thái:** Đã hoàn thành
- **Mục tiêu:** Đo đạc được thời gian mạch khởi tạo thành công thẻ SD và hiển thị/truyền qua CAN bus. Đồng thời loại bỏ tiếng tạp âm (pop noise) phát ra loa khi mạch vừa khởi động hoặc thao tác rút/cắm thẻ bằng chân MUTE của Amply (PA4).
- **Kịch bản kiểm thử :**
  1. Cấp nguồn hoặc bấm Reset hệ thống. Kết quả mong đợi: Loa hoàn toàn im lặng, không có tiếng bụp.
  2. Rút thẻ SD và cắm lại nóng. Kết quả mong đợi: Đèn LED hiển thị thời gian kết nối (ms), loa im lặng tuyệt đối không xẹt nhiễu, sau đó báo 2 bíp và phát nhạc bình thường. Dữ liệu trên CAN sniffer báo đúng BCD frame `0x125`.

---

### Task 6: Lưu và khôi phục trạng thái phát nhạc khi nóng-rút thẻ (Hot-plug Resume)
- **Trạng thái:** Đã hoàn thành
- **Mục tiêu:** Mạch phải tự động lưu lại điểm dừng của bài hát đang phát nếu người dùng rút thẻ nhớ đột ngột. Khi cắm thẻ vào lại, mạch tự động tìm file cũ và phát tiếp từ điểm bị ngắt thay vì hát lại từ đầu. Hỗ trợ ghi thẻ RFID `PHAT_NHAC` thông qua câu lệnh CANBus ID `0x204`.
- **Kịch bản kiểm thử:**
  1. Cho chạy file nhạc dài `MUSIC.MP3` bằng thẻ RFID. Đột ngột rút thẻ nhớ.
  2. Cắm thẻ nhớ lại. Kết quả mong đợi: Bản nhạc tự động nhảy tới đúng mili-giây bị ngắt và hát nối tiếp mượt mà.

---

## 3. Cải thiện canbus

### Task 7: Tối ưu mạng CAN và Cảnh báo lỗi
- [x] Tối ưu hóa băng thông (chỉ gửi frame định kỳ/sự kiện).
- [x] Kích hoạt chế độ `AutoBusOff` (Tự phục hồi sau khi đứt dây).
- [x] Thêm file `database.dbc` để hiển thị tiếng Việt rõ ràng trên Cangaroo thay vì mã Hex.
- [x] Thêm cảnh báo thông minh bằng còi và chớp `----` khi đứt cáp, tự dọn lỗi khi cắm lại.


---

## 4. Quản lý tài nguyên

### Task 8: Đánh giá và tối ưu tài nguyên Chip 
- **Trạng thái:** Chưa bắt đầu 
- **Mục tiêu:** Đo đạc mức sử dụng cpu, mức độ sử dụng RAM và Stack để đảm bảo mạch không bao giờ bị  quá tải hoặc tràn bộ nhớ.
- **Kịch bản kiểm thử (Test Cases):**
  1. Gắn các bộ đếm vào Main Loop và SystemTimer để tính toán % thời gian CPU rảnh rỗi. Kết quả mong đợi: Idle time > 60% khi hoạt động full tải (vừa đọc thẻ, vừa quẹt RFID, vừa chạy MP3).
