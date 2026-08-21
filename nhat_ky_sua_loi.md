Tài liệu này ghi chép chi tiết nguyên nhân gốc rễ và cách khắc phục cho từng vấn đề đã được giải quyết trong quá trình hoàn thiện Firmware theo yêu cầu đánh giá của cấp trên.

---

## 1. Lỗi quẹt đè thẻ làm gián đoạn âm thanh (Đã khắc phục)

### Tình trạng ban đầu:
- Khi người dùng quẹt thẻ liên tục hoặc có nhiều sự kiện phát sinh cùng lúc, âm thanh cũ bị ngắt ngay lập tức để phát âm thanh mới, dẫn đến câu nói bị cắt ngang.

### Nguyên nhân cốt lõi:
- Kiến trúc cũ gọi trực tiếp f_close() và f_open() để thay thế file âm thanh ngay lập tức khi hàm mp3.play() được gọi, không có cơ chế chờ đợi âm thanh hiện tại kết thúc.

### Cách khắc phục (Solution):
- **Thiết kế Audio Queue (Hàng đợi âm thanh):** 
  - Khởi tạo một bộ đệm vòng (Circular Queue) với sức chứa tối đa 5 file.
  - Khi có lệnh phát âm thanh, hệ thống chỉ đẩy tên file vào hàng đợi thay vì phát ngay lập tức.
  - Hệ thống tự động kiểm tra trạng thái chip giải mã VS1011E trong vòng lặp chính. Nếu chip đang rảnh rỗi (!isPlaying) và hàng đợi có phần tử, hệ thống mới tiến hành trích xuất bài tiếp theo để phát.
  - Nhờ cơ chế này, các luồng âm thanh được phát nối tiếp nhau một cách tự nhiên, giải quyết triệt để vấn đề âm thanh bị đè/cắt ngang.

---

## 2. Lưu giá trị bộ đếm khi mất nguồn (Đã khắc phục)

### Tình trạng ban đầu:
- Giá trị bộ đếm sản phẩm (Sensor::productCount) bị reset về 0 mỗi khi mạch bị ngắt nguồn, gây mất dữ liệu sản xuất.

### Nguyên nhân cốt lõi:
- Biến đếm chỉ được lưu trong RAM thông thường của vi điều khiển. RAM này sẽ xóa sạch dữ liệu ngay khi điện áp VDD tụt xuống 0V. Trước đây chưa cấu hình bộ nhớ lưu trữ bền vững (như EEPROM, Flash, hay Backup Register).

### Cách khắc phục (Solution):
- **Sử dụng Thanh ghi dự phòng (Backup Register - BKP):**
  - Kích hoạt nguồn và Clock cho phân vùng Backup của STM32.
  - Lưu giá trị biến đếm thẳng vào thanh ghi BKP->DR1 mỗi khi có cảm biến kích hoạt đếm lên, hoặc khi nhận lệnh reset/cập nhật qua mạng CAN.
  - Mỗi khi mạch khởi động lại, lấy BKP->DR1 nạp ngược lại cho Sensor::productCount.
  - Ưu điểm: Thanh ghi BKP được nuôi bằng nguồn Pin dự phòng (VBAT). Tránh được tình trạng mòn phần cứng.

---

## 3. Cảm biến bắt thiếu tín hiệu khi lướt nhanh (Đã khắc phục)

### Tình trạng ban đầu:
- Khi đưa vật cản qua cảm biến với tốc độ cao (ví dụ: đưa nhanh 3 lần liên tiếp), bộ đếm chỉ nhảy 2 lần (bị lọt/mất 1 lần đếm).

### Nguyên nhân cốt lõi:
- Thuật toán chống rung (Debounce) bằng phần mềm của ngắt EXTI đang bị cài đặt ở mức quá cao là 200ms.

### Cách khắc phục (Solution):
- **Giảm thời gian Debounce và viết bộ Diagnostic (Logic Analyzer):**
  - Hạ thời gian chống rung từ 200ms xuống còn 20ms.
  - Cấu hình ngắt EXTI bắt cả cạnh lên và cạnh xuống.
  - Phát hiện cảm biến là loại PNP (Active HIGH). Đã xây dựng một Máy trạng thái kèm lọc nhiễu.

---

## 4. Nâng cấp cơ chế nhận diện và tương thích mọi loại thẻ nhớ SD (Đã khắc phục)

### Tình trạng ban đầu (Nguyên nhân):
- **Nhận diện chậm & Tốn tài nguyên:** Mã nguồn cũ không tận dụng chân báo thẻ vật lý PB0 (SD_DETECT), mà sử dụng cơ chế Polling (hỏi vòng liên tục), gây lãng phí CPU và phản hồi chậm khi rút/cắm thẻ nóng (hot-plug).
- **Thiếu quản lý nguồn điện:** Chân PB1 (SD_PWR_EN) nối với Transistor PNP cấp nguồn cho thẻ bị bỏ trống. Việc cấp điện liên tục khiến thẻ không được làm mới (hard-reset) trước mỗi lần khởi động.
- **Kén thẻ dung lượng cao:** Mạch chỉ nhận mượt các thẻ chuẩn cũ (dưới 2GB). Với thẻ 16GB, 64GB (SDHC/SDXC), vi điều khiển nội bộ của thẻ cần nhiều thời gian để nạp điện hơn. Thư viện SPI cũ có timeout quá ngắn khiến thẻ chưa kịp phản hồi đã bị kết luận là lỗi phần cứng (báo **5 tiếng bíp**). Ngược lại, nếu thẻ nhận phần cứng thành công nhưng sai định dạng (ví dụ 64GB hay bị format nhầm thành exFAT/NTFS) thì FatFs không thể đọc được phân vùng, dẫn đến lỗi File System (báo **3 tiếng bíp**).

### Cách khắc phục (Solution):
1. **Vận dụng ngắt cứng PB0 (SD_DETECT) & State Machine:**
   - Cấu hình PB0 làm ngắt ngoài (EXTI0). Ngay khi cắm thẻ (PB0 kéo xuống LOW), ngắt kích hoạt lập tức.
   - Bổ sung bộ lọc nhiễu phần mềm (đọc PB0 liên tiếp 10 lần) để loại bỏ sóng nhiễu từ RFID.
   - Thiết lập State Machine (`sdCardInSlot`) đảm bảo mạch chỉ khởi tạo 1 lần duy nhất cho mỗi lần cắm, tuyệt đối không bị đơ/treo hệ thống kể cả khi người dùng cắm thẻ hỏng.
2. **Điều khiển cấp nguồn độc lập qua PB1 (SD_PWR_EN):**
   - Lập trình lại hàm `SdCard::powerOn()` và `powerOff()`. Kéo PB1 xuống LOW để mở Transistor PNP cấp nguồn, và đưa lên HIGH để ngắt nguồn.
   - Giúp thẻ được hard-reset và xóa sạch trạng thái lỗi trước khi bắt đầu khởi tạo lại.
3. **Mở rộng tương thích thư viện cho mọi loại thẻ (SDSC, SDHC, SDXC):**
   - Ép kiểu truy cập thanh ghi `SPI->DR` thành 8-bit `(volatile uint8_t*)` để sửa lỗi phần cứng SPI của STM32 truyền thừa xung nhịp.
   - Nới lỏng giới hạn Timeout khởi tạo: Tăng vòng lặp chờ lệnh CMD0 lên 1 giây, và lệnh ACMD41 lên 2 giây. Thêm độ trễ 300ms sau khi cấp điện (PB1) để tụ điện trên các thẻ lớn (64GB) sạc đầy đủ. 
   - Đồng bộ định dạng file: Yêu cầu định dạng bắt buộc là FAT32 (dùng GUIFormat cho thẻ 64GB) vì chuẩn thư viện nội bộ không hỗ trợ exFAT.

### Kết quả (Result):
- Mạch tương thích hoàn hảo với mọi dung lượng thẻ từ 2GB đến 64GB (khi dùng đúng chuẩn FAT32).
- Khôi phục thành công tính năng **chẩn đoán thẻ thông minh (Diagnostic Beeps)**. Hệ thống không còn bị treo hay bíp chồng chéo loạn xạ mà báo lỗi cực kỳ chuẩn xác và duy nhất 1 lần:
  - **Cắm thẻ tốt:** Báo 2 tiếng bíp, mạch sẵn sàng phát nhạc.
  - **Cắm thẻ sai định dạng (exFAT/NTFS):** Báo 3 tiếng bíp rồi nghỉ. 
  - **Cắm thẻ hỏng:** Báo 5 tiếng bíp rồi nghỉ.
- Dù cắm thẻ hỏng hay thẻ tốt, CPU vẫn không bị chiếm dụng, nhường 100% tài nguyên để quẹt thẻ RFID luôn mượt mà. Đạt chuẩn tính năng Plug & Play siêu mượt.

---

## 5. Đo tốc độ nạp thẻ và triệt tiêu tiếng bụp loa (Đã khắc phục)

### Tình trạng ban đầu:
- **Loa kêu bụp bụp (Pop noise):** Khi bật nguồn khởi động hoặc khi cắm/rút thẻ nhớ nóng, loa phát ra những tiếng bụp rất to gây khó chịu.
- **Không có dữ liệu đo tốc độ:** Hệ thống nhận diện thẻ tốt, nhưng không rõ tốc độ giao tiếp (Mount Time) giữa vi điều khiển và các loại thẻ lớn nhỏ (16GB, 64GB) diễn ra mất bao nhiêu mili-giây, khó tối ưu thêm.

### Nguyên nhân cốt lõi:
- **Tiếng bụp:** Mỗi lần khởi động mạch hoặc thao tác cắm/rút thẻ, thư viện gọi lệnh `mp3.init()` để hard-reset vi mạch VS1053. Quá trình reset này làm sụt/tăng áp đột ngột ở mạch DAC nội bộ của chip, tạo ra xung điện lọt ra loa. Chân `PA4` (Amply Enable) được cấu hình bật ngay lập tức từ đầu nên không chặn được tạp âm này.

### Cách khắc phục (Solution):
1. **Theo dõi tốc độ thẻ SD (Mount Time):**
   - Lập trình bộ tính thời gian `SystemTimer::getTick()` để đo chính xác thời gian từ lúc mạch phát hiện chân thẻ SD đến lúc báo `f_mount` thành công.
   - Hiển thị trực tiếp tốc độ (ms) lên màn hình LED 7 thanh trong vòng 3 giây để dễ dàng quan sát, sau đó tự quay lại màn hình đếm sản phẩm.
   - Chuyển đổi con số thời gian sang mã **BCD (Binary-Coded Decimal)** và truyền qua mạng CAN (ID `0x125`) với độ dài 4 byte. Điều này giúp phần mềm CAN Monitor trên máy tính hiển thị dữ liệu trực quan y hệt như số thập phân.
2. **Triệt tiêu âm bụp (Mute Amply qua chân PA4):**
   - Đảo trạng thái mặc định của chân `PA4` trong hàm `MX_GPIO_Init()` sang `GPIO_PIN_SET` (Tắt Amply) để chặn ngay tiếng bụp khi mạch vừa được cấp nguồn/bấm nút Reset.
   - Xây dựng cơ chế Mute động trong vòng lặp cắm nóng thẻ: Tắt Amply -> Reset chip MP3 & Thẻ nhớ -> Đợi 100ms cho điện áp triệt tiêu -> Bật lại Amply.

### Kết quả (Result):
- Mạch hoạt động êm ái hoàn toàn, không còn bất kỳ tiếng bụp hay tạp âm nào từ loa trong mọi hoàn cảnh (từ lúc cấp nguồn, reset, cho đến tháo lắp thẻ nhớ liên tục).
- Hệ thống theo dõi thẻ (Diagnostic) nay đã hoàn thiện với khả năng báo bíp thông minh và theo dõi tốc độ thẻ qua mạng CAN.

---

## 6. Tính năng khôi phục âm thanh dở dang khi rút cắm thẻ (Hot-plug Resume) (Đã hoàn thành)

### Tình trạng ban đầu:
- Khi người dùng đang nghe dở một file nhạc mà đột ngột rút thẻ nhớ ra, bản nhạc bị ngắt. Khi cắm thẻ vào lại, mạch chỉ khởi tạo lại thẻ và đứng im. Người dùng phải quẹt thẻ để bắt đầu nghe lại từ đầu, rất bất tiện đối với các file âm thanh dài.

### Nguyên nhân cốt lõi:
- Ở cấu trúc thư viện cũ, luồng xử lý âm thanh không có cơ chế ghi nhớ. Khi thẻ bị rút, hệ thống lập tức xóa bỏ và đóng toàn bộ thông tin về bài hát đang phát (bao gồm tên bài hát và thời lượng đang phát). Do đó, khi thẻ được cắm lại, hệ thống hoàn toàn "trắng bộ nhớ" nên không biết phải phát tiếp bài nào và từ đâu.

### Cách khắc phục (Solution):
- **Tạo bộ nhớ đệm (State Memory):** Bổ sung các biến để lưu trữ tên bài hát (`savedFilename`) và vị trí đang phát (`savedPosition`) vào thư viện `Mp3`.
- **Lưu trạng thái tự động:** Khi người dùng rút thẻ nhớ, hệ thống sẽ lập tức lưu lại tên bài hát và lấy vị trí con trỏ file hiện tại (thông qua lệnh `f_tell()`) để cất vào RAM trước khi ngắt hoàn toàn kết nối thẻ nhớ.
- **Khôi phục bài hát tự động:** Khi thẻ được cắm lại và ngàm giữ thẻ báo tín hiệu kết nối ổn định, hàm `mp3.resumeState()` sẽ được kích hoạt. Mạch tự động mở lại file nhạc đã lưu, dùng lệnh `f_lseek()` để tua file đến đúng vị trí byte bị đứt gãy trước đó, rồi tự động phát tiếp ra loa.

### Kết quả (Result):
- Tính năng phát lại nhạc hoạt động hoàn hảo. Nếu đang nghe dở file nhạc dài mà thẻ nhớ bị rút ra, lúc cắm lại bài hát sẽ tự động chạy tiếp nối liền đúng khoảnh khắc vừa bị ngắt mà không cần bất kỳ thao tác thủ công nào.
