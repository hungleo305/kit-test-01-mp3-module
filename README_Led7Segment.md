# Sổ Tay Toàn Tập Thư Viện Led7Segment (C++)

Phần này hướng dẫn sử dụng thư viện cho các loại module led 7 đoạn dùng ic dịch chốt

---

## PHẦN 1: CHI TIẾT TỪNG HÀM LỆNH API

Dưới đây là giải thích rõ toàn bộ các hàm được cung cấp bởi `Led7Segment`.

### 1.1 Nhóm Khởi tạo & Cấu hình phần cứng

**`Led7Segment(GPIO_TypeDef* dPort, uint16_t dPin, GPIO_TypeDef* clkPort, uint16_t clkPin, GPIO_TypeDef* strPort, uint16_t strPin, uint8_t digits)`**
- **Chức năng:** Hàm khởi tạo (Constructor). Gán chân điều khiển và khai báo số lượng đèn LED sẽ điều khiển.
- **Tham số:** Các cặp Port và Pin của ngõ Data (DS), Clock (SH_CP), và Strobe/Latch (ST_CP). `digits` là tổng số con LED 7 đoạn nối tiếp nhau (ví dụ mạch có 9 con LED thì truyền `9`).

**`void init()`**
- **Chức năng:** Tự động kích hoạt xung nhịp (Clock Enable) cho các Port GPIO tương ứng và thiết lập cấu hình điện áp ngõ ra (Output Push-Pull) tốc độ cao cho 3 chân tín hiệu. **Bắt buộc** gọi 1 lần trước vòng lặp chính `while(1)`.

**`void setSegmentMap(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t e, uint8_t f, uint8_t g, uint8_t dp)`**
- **Chức năng:** Vẽ lại sơ đồ chân. Dùng để bẻ luồng dữ liệu phần mềm cho khớp với cách đi dây của thợ thiết kế mạch phần cứng.
- **Tham số:** Truyền vào số thứ tự bit (từ 0 đến 7) mà IC đang cấp điện cho thanh nét tương ứng (A, B, C...).

**`void setReverseMask(uint32_t mask)`**
- **Chức năng:** Lật ngược hiển thị 180 độ bằng phần mềm đối với những con LED bị cắm lộn đầu trên bo mạch.
- **Tham số:** Một biến mặt nạ cờ bit. Set bit thứ `n` lên 1 để lộn ngược con LED ở vị trí `n`. (Ví dụ vị trí 0 và 1 bị lộn thì truyền `(1<<0) | (1<<1)`).

**`void setInverted(bool state)`**
- **Chức năng:** Đảo ngược logic phát sáng toàn bộ màn hình. 
- **Tham số:** `true` nếu dùng mạch Anode chung (mức THẤP - 0 là sáng). `false` nếu dùng mạch Cathode chung (mức CAO - 1 là sáng). Mặc định là `false`.

**`void setStrobeOE(bool state)`**
- **Chức năng:** Bật tính năng điều khiển chân `OE` (Output Enable) để tắt màn hình khẩn cấp chống nhiễu phần cứng. Nếu đặt là `true`, thư viện sẽ ngầm hiểu chân Strobe/Latch kiêm luôn cả nhiệm vụ dập điện OE.

### 1.2 Nhóm Vẽ Đệm (Thay đổi mảng RAM)

Các hàm này hoàn toàn KHÔNG tác động tới chân vi điều khiển, mà chỉ thay đổi bộ nhớ tạm (Buffer ảo).

**`void clearBuffer()`**
- **Chức năng:** Điền giá trị mờ (0) vào toàn bộ mảng đệm, tương đương với việc "xóa bảng" để chuẩn bị vẽ một khung hình mới. Luôn gọi trước khi dùng `setDigit` hoặc `setNumber`.

**`void setDigit(uint8_t index, uint8_t val, bool showDot = false)`**
- **Chức năng:** Đặt một chữ số đơn lẻ vào một vị trí LED được chỉ định.
- **Tham số:**
  - `index`: Vị trí LED tính từ 0. (Thường quy ước `0` là LED ngoài cùng bên trái và đếm dần sang phải, nhưng thực tế hoàn toàn phụ thuộc vào thợ vẽ mạch nối con IC đầu tiên nằm ở bên nào).
  - `val`: Giá trị số từ 0 đến 9. Nếu truyền `0xFE`, thư viện sẽ tự hiểu là bạn muốn vẽ dấu gạch ngang báo lỗi (`-`).
  - `showDot`: truyền `true` để bật sáng thêm dấu chấm nhỏ ở góc dưới cùng bên phải của chữ số đó.

**`void setNumber(uint32_t number, uint8_t startIndex, uint8_t length, bool padWithZeros)`**
- **Chức năng:** Tự động cắt 1 số dài (như 1234) ra từng chữ số rồi xếp lần lượt vào dải đèn LED.
- **Tham số:**
  - `num`: Con số muốn hiển thị.
  - `startIndex`: Vị trí LED bắt đầu vẽ (tính từ trái qua).
  - `length`: Chiều dài (số lượng) LED mà bạn dùng để chứa con số này.
  - `padWithZeros`: truyền `true` sẽ chèn số `0` ở những khoảng trống phía trước (ví dụ số `5`, chiều dài 3 -> tự động vẽ ra `005`). `false` sẽ để trống (đèn LED tắt đen) ở các khoảng đó.

**`void setDot(uint8_t index, bool showDot)`**
- **Chức năng:** Bật hoặc tắt ĐỘC LẬP dấu chấm nhỏ ở một chữ số mà KHÔNG làm thay đổi (hoặc xóa mất) con số đang hiển thị ở vị trí đó.
- **Tham số:** 
  - `index`: Vị trí đèn LED muốn bật/tắt dấu chấm.
  - `showDot`: truyền `true` để bật, `false` để tắt dấu chấm.
  - *Lưu ý:* Hàm này cực kỳ hữu dụng để làm hiệu ứng chớp tắt dấu chấm (nhịp giây, cảnh báo lỗi) đè lên trên lớp dữ liệu đếm/thời gian mà không làm nháy chớp con số.

### 1.3 Nhóm Đẩy Dữ Liệu (Giao tiếp phần cứng)

**`void refresh()`**
- **Chức năng:** Lệnh cốt lõi quan trọng nhất! Đọc lại toàn bộ nội dung mảng đệm trong RAM, bóp méo hình ảnh qua các lớp bộ lọc mặt nạ (Segment Map, Reverse Mask, Invert) mà bạn đã cài đặt ở Phần 1, sau đó "bơm" từng dòng điện (bit-bang SPI) ra vi mạch 74HC595 và chốt Latch để ép đèn LED phát sáng.

**`void showDashes()`**
- **Chức năng:** Hàm tiện ích "làm biếng". Tự động xóa trắng bảng, điền dấu gạch ngang (`-`) vào toàn bộ các con LED và gọi `refresh()` ngay lập tức. Rất hữu dụng khi cần báo lỗi mất kết nối cảm biến.

**`void blank()`**
- **Chức năng:** Hàm tiện ích. Xóa trắng bảng và gọi `refresh()` để tắt đen toàn bộ màn hình một cách an toàn.

**`void hide()`**
- **Chức năng:** Hàm đóng cầu dao phần cứng SIÊU TỐC. Nếu `setStrobeOE(true)` đã được bật, hàm này không thèm quan tâm tới dữ liệu SPI hay Buffer. Nó trực tiếp kéo chân Strobe lên mức CAO để cắt điện của IC LED ngay lập tức. Dùng riêng biệt để chống nhiễu (EMI) khi đóng ngắt các động cơ, công tắc lớn.

### 1.4 Nhóm Gỡ Lỗi

**`void autoScan(uint32_t delayMs)`**
- **Chức năng:** Chạy vòng lặp tự động nhích bật từng thanh sáng duy nhất dọc theo toàn bộ hệ thống IC. Giúp mắt người chậm rãi nhìn và dò tìm cách cắm dây chân phần cứng trên board lạ mà không treo hệ thống máy.
- **Tham số:** Thời gian chờ (delay không gây treo mạch bằng HAL_GetTick) giữa 2 lần sáng đèn kế tiếp nhau (thường dùng `1000` tương đương 1 giây).

---

## PHẦN 2: HƯỚNG DẪN SỬ DỤNG CHUNG VÀ VÍ DỤ

Khi thiết kế hoặc mua một module LED mới, thường nó sẽ không hiển thị đúng ngay lần đầu tiên do được đi dây ngẫu nhiên để tối ưu diện tích bo mạch. Đừng sửa nối dây phần cứng! Thư viện có đủ các hàm để điều chỉnh phần mềm cho khớp với phần cứng.

### 2.1 Khai báo thư viện và khởi tạo ban đầu
```cpp
// Khai báo đối tượng (Nên để ở Global hoặc đầu file main.cpp)
// Thông số: Chân Data, Chân Clock, Chân Latch/Strobe, Tổng số Digit (vd: 9)
Led7Segment dp(GPIOA, GPIO_PIN_4, GPIOA, GPIO_PIN_5, GPIOA, GPIO_PIN_6, 9);

// Trong hàm setup() hoặc trước while(1)
dp.init(); 
```

### 2.2 Hiển thị rác, số lộn xộn (Sai Segment Map)
**Hiện tượng:** Truyền lệnh hiển thị số `1`, nhưng module led lại hiển thị loạn, không đúng yêu cầu
**Khắc phục:** Sử dụng hàm `autoScan(1000)` để dò chân.
```cpp
while(1) {
    // Đặt dòng này vào vòng lặp vô hạn. Mỗi giây nó sẽ dịch 1 bit duy nhất.
    // Dùng mắt nhìn xem mạch LED đang sáng thanh nào theo thứ tự.
    dp.autoScan(1000); 
}
```
Giả sử khi chạy hàm `autoScan`, bạn thấy thứ tự đèn sáng là: **C -> D -> E -> F -> G -> DP -> A -> B**.
Điều này có nghĩa là Bit 0 của IC đang nối với thanh C, Bit 1 nối với thanh D... Ghi lại vào hàm `setSegmentMap` theo đúng thứ tự nhìn thấy:
```cpp
// Thứ tự tham số là: A, B, C, D, E, F, G, DP
// Ở ví dụ trên, thanh A sáng ở nhịp số 6, thanh B sáng ở nhịp số 7...
dp.setSegmentMap(6, 7, 0, 1, 2, 3, 4, 5); 
```

### 2.3 Hiển thị ngược số 180 độ (Sai Reverse Mask)
**Hiện tượng:** Trong một module Led có digits hiển thị ngược 180 độ do phần cứng cắm ngược đầu (Chữ số 2 trông giống chữ số 5).
**Khắc phục:** Đếm xem con LED đó nằm ở vị trí nào (tính từ 0). Giả sử con số 0 và con số 1 bị ngược.
```cpp
// 1<<0 nghĩa là đánh dấu LED số 0 bị lộn ngược
// 1<<1 nghĩa là đánh dấu LED số 1 bị lộn ngược
dp.setReverseMask((1<<0) | (1<<1));
```
Thư viện sẽ tự động tính toán bù trừ trên phần mềm để lật ngược hình ảnh lại.

### 2.4 Bật số 1 thì LED tối, gửi số 0 thì LED sáng (Sai Invert)
**Hiện tượng:** Module led là loại Anode chung (sáng ở mức thấp).
**Khắc phục:** 
```cpp
dp.setInverted(true); // Đảo ngược logic phát sáng
```

---

### 2.5 Xóa đệm và hiển thị 
Luồng chạy **bắt buộc** phải tuân theo 4 bước sau:
`Xóa đệm (Clear)` -> `Tô vẽ dữ liệu` -> `Tô vẽ thêm hiệu ứng` -> `Bơm ra màn hình (Refresh)`

> [!WARNING]
> Mọi hàm như `setNumber`, `setDigit` chỉ đang thay đổi một biến mảng RAM trên vi điều khiển. Đèn LED phần cứng sẽ **KHÔNG HỀ THAY ĐỔI** cho đến khi bạn gọi lệnh `refresh()`.

### 2.6 Ví dụ hiển thị số đồng hồ
Bạn muốn hiển thị số 05:09 ở 4 con LED đầu tiên. 
```cpp
while(1) {
    // Chỉ cập nhật 2 lần/giây (500ms) để màn hình mượt mà không chớp nháy (flicker)
    static uint32_t lastDraw = 0;
    if (HAL_GetTick() - lastDraw >= 500) {
        lastDraw = HAL_GetTick();

        dp.clearBuffer(); // 1. Xóa sạch khung hình cũ

        // 2. Vẽ giờ (VD: số 5). Bắt đầu từ LED 0, dài 2 LED, CÓ chèn số 0 ở đầu
        dp.setNumber(5, 0, 2, true);  // => Vẽ ra "05"
        
        // 3. Vẽ phút (VD: số 9). Bắt đầu từ LED 2, dài 2 LED, CÓ chèn số 0 ở đầu
        dp.setNumber(9, 2, 2, true);  // => Vẽ ra "09"
        
        // 4. Vẽ dấu hai chấm (Dấu chấm ở LED số 1)
        dp.setDot(1, true); // Chỉ bật dấu chấm, không ảnh hưởng đến số 5 đã vẽ ở trên

        dp.refresh(); // 5. Lệnh thực thi: Bơm dòng chữ "05.09" ra màn hình!
    }
}
```

### 2.7 Chớp nháy dấu hai chấm của đồng hồ
Để làm cho dấu chấm nháy chớp theo chu kỳ, ta lợi dụng bộ đếm `HAL_GetTick()`.

```cpp
bool chop_tat = (HAL_GetTick() / 500) % 2 == 0; 
// Biến này sẽ luân phiên true/false mỗi 500ms

dp.clearBuffer();
uint16_t timeValue = 1230; // 12 giờ 30 phút
dp.setNumber(timeValue, 0, 4, true); 

// Gọi hàm setDot để bật/tắt dấu chấm độc lập mà không cần quan tâm/tính toán lại chữ số đang hiển thị
dp.setDot(1, chop_tat);
dp.setDot(2, chop_tat);
dp.refresh();
```

### 2.8 Hiện dấu gạch ngang báo lỗi (Dứt cáp, mất kết nối)
```cpp
if (loi_cam_bien) {
    dp.clearBuffer();
    // Bắn mã 0xFE (gạch ngang) vào từng LED
    for (int i = 0; i < 4; i++) {
        dp.setDigit(i, 0xFE, false);
    }
    dp.refresh();
    // Hoặc đơn giản hơn: gọi thẳng dp.showDashes();
}
```

---

### 2.9 Sử dụng chân OE chống nhiễu (EMI) và tắt nhanh màn hình

Khi vi điều khiển đóng ngắt các thiết bị tải công suất lớn (như Relay, contactor, động cơ, cuộn hút...), tia lửa điện hoặc xung ngược sinh ra có thể gây nhiễu (EMI) vào đường truyền SPI mềm của cụm LED, khiến màn hình bị "rác" ký tự. Để khắc phục triệt để, phần cứng của bạn nên được nối chân `OE` (Output Enable) của IC dịch vào một chân GPIO (ví dụ gọi chung là chân Strobe).

> [!IMPORTANT]
> Chân OE hoạt động như một công tắc tổng của toàn bộ IC dịch. Kéo mức CAO là cắt điện LED ngay lập tức, kéo mức THẤP là cho phép hiển thị trở lại.

**Quy trình chuẩn để đóng ngắt tải nặng chống nhiễu:**
```cpp
// Đầu chương trình:
dp.setStrobeOE(true); // Thông báo thư viện có dùng chức năng OE ở chân Strobe

// ... Trong luồng xử lý sự kiện:
if (can_kham_tai) {
    dp.hide();          // 1. Tắt nhanh toàn bộ LED phần cứng lập tức (ngắt OE)
    
    kich_hoat_tai();    // 2. Kích tải nặng (Relay, Động cơ)! Nhiễu điện sinh ra lúc này.
    HAL_Delay(30);      // 3. Chờ một khoảng nhỏ (ví dụ 30ms) cho nhiễu suy hao hết.
    
    dp.refresh();       // 4. Bơm lại dữ liệu đệm trong RAM ra IC và tự động bật lại OE. Màn hình phục hồi bình thường!
}
```
