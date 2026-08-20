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

### Task 3: Tăng độ nhạy ngắt cảm biến 
- **Trạng thái:** Đã hoàn thành
- **Mục tiêu:** Cảm biến phải bắt được vật thể lướt qua với tốc độ cực cao, không bị điểm mù do thuật toán chống rung (Debounce) bằng phần mềm gây ra.
- **Kịch bản kiểm thử :**
  1. Dùng tay (hoặc vật cản) lướt qua cảm biến cực nhanh 2 - 3 lần liên tục. Kết quả mong đợi: Còi bíp đủ số lần, bộ đếm nhảy đủ số lượng.
  2. Bật chế độ "Stress Test" trong mã nguồn ,kết quả mong đợi: Gói tin CAN ID `0x123` đẩy lên Cangaroo chỉ ra MCU có thể xử lý thành công ~370.000 sự kiện/giây.

---

## 2. Nâng cấp module thẻ nhớ SD & Audio (SD Card & MP3)

### Task 4: Tối ưu cơ chế nhận diện và kết nối thẻ SD
- **Trạng thái:** Chưa bắt đầu 
- **Mục tiêu:** Tự động phát hiện thẻ bị rút/cắm vào ngay lập tức (Zero Delay) thông qua chân tín hiệu phần cứng Detect (PB0) thay vì phải liên tục gọi hàm Polling . Đảm bảo thư viện hỗ trợ tốt mọi loại thẻ SD.
- **Kịch bản kiểm thử :**
  1. Đang chạy bình thường, rút thẻ SD ra. Kết quả mong đợi: Còi báo hiệu thẻ lỗi ngay lập tức (không cần đợi vài giây).
  2. Cắm thẻ SD vào lại. Kết quả mong đợi: Mạch tự động Mount lại File System ngay lập tức và sẵn sàng đọc file.

### Task 5: Hot-plug thẻ nhớ không gián đoạn âm thanh
- **Trạng thái:** Chưa bắt đầu 
- **Mục tiêu:** Nếu người dùng rút thẻ SD trong lúc đang phát nhạc và cắm lại rất nhanh, hệ thống phải lưu được con trỏ file (File Pointer) và tiếp tục phát ở đúng giây bị gián đoạn.
- **Kịch bản kiểm thử :**
  1. Cho mạch phát một file mp3 dài khoảng 20 giây. Đến giây thứ 5, rút thẻ SD ra và cắm lại ngay. Kết quả mong đợi: Bài nhạc tiếp tục phát từ khoảng giây thứ 5.

---

## 3. Cải thiện canbus

### Task 6: Tối ưu & hiển thị trạng thái mạng CAN Bus
- **Trạng thái:** Chưa bắt đầu
- **Mục tiêu:** Tối ưu hóa băng thông mạng CAN, giảm tỷ lệ lỗi gói tin và hiển thị cảnh báo rõ ràng khi mạng bị mất kết nối. Thêm hiển thị rõ ràng các giá trị được gửi và nhận có nghĩa là gì thay vì chỉ mỗi giá trị


---

## 4. Quản lý tài nguyên

### Task 7: Đánh giá và tối ưu tài nguyên Chip 
- **Trạng thái:** Chưa bắt đầu 
- **Mục tiêu:** Đo đạc mức sử dụng cpu, mức độ sử dụng RAM và Stack để đảm bảo mạch không bao giờ bị  quá tải hoặc tràn bộ nhớ.
- **Kịch bản kiểm thử (Test Cases):**
  1. Gắn các bộ đếm vào Main Loop và SystemTimer để tính toán % thời gian CPU rảnh rỗi. Kết quả mong đợi: Idle time > 60% khi hoạt động full tải (vừa đọc thẻ, vừa quẹt RFID, vừa chạy MP3).
