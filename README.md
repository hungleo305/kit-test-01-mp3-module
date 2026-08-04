# Kit test 01 - Kết nối MP3

## Mô tả
Bài tập nhằm mục đích kiểm tra khả năng nghiên cứu và tích hợp các module cơ bản thành 01 project lớn hơn.
## Phần cứng cung cấp
- 01 Mạch điều khiển + 01 mạch nạp
- 01 thẻ nhớ micro sd + 01 đầu đọc thẻ
- 01 module RFID + 03 thẻ RFID
- 01 cảm biến (đã được hàn vào mạch)
- 01 nguồn 9 VDC
- 01 module led 7 đoạn
- 01 speaker
- 01 module canbus

## Yêu cầu chung
- Tất cả code được viết trên IDE IAR (có thể sử dụng CubeMX để gen code ban đầu)
- Viết code kiểu truy cập thanh ghi (**hạn chế** sử dụng thư viện HAL)
- Code bắt buộc phải viết thành kiểu **class** để có thể tái sử dụng sau này
- **Led main** trên mạch bắt buộc phải nháy với chu kỳ 100ms ON/ 100ms OFF trong bất kỳ trường hợp nào để biết được là mạch đang hoạt động.
- Khi hệ thống mới khởi động: còi chip cần kêu 2 lần (50ms ON, 100ms OFF) để báo hiệu là mạch bắt đầu hoạt động

## Tính năng cần có
- Cảm biến: khi có tín hiệu từ cảm biến thì bộ đếm của mạch tăng 01 đơn vị
- Module led 7 đoạn: mỗi 5 giây thì hiển thị giá trị bộ đếm, sau đó đến thời gian thực (hh:mm) (dấu 2 ), tuần tự theo chu kỳ
- Module RFID: 
    - Có thể đọc/ghi/xoá/bảo mật thẻ thông qua đường vật lý CANBUS (tự đề xuất phương pháp - frame để làm điều này)
    - Thẻ 01: khi quẹt -> Còi chip kêu 01 lần, phát âm thanh ra loa : "Reset giá trị về 0" và vi điều khiển thực hiện reset giá trị đếm hiện tại về 0
    - Thẻ 02: khi quẹt -> Còi chip kêu 01 lần, phát âm thanh "Bật relay" -> sau đó chip bật relay trên mạch
    - Thẻ 03: khi quẹt -> Còi chip kêu 01 lần, phát âm thanh "Đảo trạng thái relay" -> sau đó chip đảo trạng thái relay trên mạch
    - Dữ liệu thẻ từ mỗi khi quẹt được thì cần truyền về PC qua chuẩn canbus (tự đề xuất phương pháp - frame để làm điều này)
- Module Canbus:
    - Đọc realtime dữ liệu từ mạch về gồm:
        - Giá trị bộ đếm hiện tại
        - Giá trị cài đặt (khi đếm đến giá trị này thì mạch phát âm thanh "Đã đạt mục tiêu")
        - Thời gian thực trên mạch
        - Dữ liệu của thẻ RFID
    - Có thể ghi/cập nhật được các dữ liệu sau đây xuống mạch (tự đề xuất phương pháp - frame để làm điều này):
        - Giá trị bộ đếm hiện tại
        - Giá trị cài đặt (target)
        - Thời gian thực
        - Thay đổi tính năng của thẻ (3 thẻ, mỗi thẻ có tính năng riêng biệt như trên mục _Module RFID_)
- Thẻ nhớ: lưu vài âm thanh cố định, định dạng âm thanh là MP3, thẻ nhớ cần định dạng FAT32 hoặc NTFS.

# Yêu cầu sản phẩm cuối
- 01 hệ thống hoạt động đầy đủ các tính năng như yêu cầu
- Giải thích được toàn bộ các dòng code mình viết
- Chỉnh sửa trực tiếp được 1 vài tính năng cơ bản khi review bài tập

# Chú ý
- Cần phải kiểm tra, đo đạc kỹ trước khi cắm điện để tránh trường hợp hỏng thiết bị, hỏng máy tính, cháy nổ, ...
- Có vấn đề gì về phần cứng thì cần phải thông báo ngay cho quản lý.

# Thời gian làm bài tập
- 02 tuần kể từ ngày nhận đề bài

# Hướng dẫn làm việc
- Tạo 1 nhánh mới tên "dev" để lập trình
- 1 ngày cập nhật ít nhất 1 lần lên cloud + git (commit dồn 1 cục vào ngày cuối sẽ tính là không đạt)
- File readme.md cần cập nhật realtime hàng ngày theo quá trình làm việc
- Chuẩn hoá phần hướng dẫn cài đặt: hướng dẫn tạo project, cách chạy, các thư viện sử dụng, ... (tham khảo các project lớn trên github)

# Hướng dẫn kết nối repo ban đầu
```
cd existing_repo
git remote add origin https://git.vnatechlab.com/thuc_tap_sinh/firmware/kit-test-01-mp3-module.git
git branch -M main
git push -uf origin main
```
