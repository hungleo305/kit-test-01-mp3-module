# Hướng dẫn sử dụng thư viện Rfid (RC522)

Thư viện `Rfid` được thiết kế để điều khiển module đọc thẻ RFID RC522 thông qua giao thức SPI (kế thừa nền tảng từ thư viện `SpiBus`). Thư viện hỗ trợ phát hiện thẻ, đọc UID (mã định danh), xác thực bảo mật và đọc/ghi dữ liệu lên các block (khối) bộ nhớ của thẻ MIFARE Classic 1K.

---

## 1. Khởi tạo đối tượng (Constructor)

```cpp
Rfid(SpiBus* spi_bus, 
     GPIO_TypeDef* csPort, uint16_t csPin, 
     GPIO_TypeDef* rstPort, uint16_t rstPin);
```

**Cách khai báo:**
Cần truyền vào con trỏ của đối tượng `SpiBus` (để dùng chung chuẩn giao tiếp SPI), cùng với các chân điều khiển CS (Chip Select) và RST (Reset).

*Ví dụ:*
```cpp
// Khởi tạo kênh SPI chung trước
SpiBus spi1(SPI1, GPIOA, GPIO_PIN_5, GPIOA, GPIO_PIN_7, GPIOA, GPIO_PIN_6);

// Khởi tạo RFID sử dụng SPI1, chân CS là PA4, chân RST là PA3
Rfid rfid(&spi1, GPIOA, GPIO_PIN_4, GPIOA, GPIO_PIN_3);
```

---

## 2. Hàm Cấu hình Hệ thống

**`void init()`**
- **Chức năng:** Kích hoạt chân GPIO cho CS/RST, tiến hành soft-reset module RC522 và thiết lập các thanh ghi (bộ định tuyến, timer, an-ten) để module sẵn sàng dò tìm thẻ ở tần số 13.56 MHz.
- **Cách dùng:** Phải gọi 1 lần duy nhất trong hàm `main()` trước vòng lặp `while(1)`.

---

## 3. Nhóm Hàm Quét & Đọc Mã Định Danh (UID)

**`bool checkCard()`**
- **Chức năng:** Phát sóng vô tuyến để tìm xem có thẻ RFID nào đang nằm trong vùng phủ sóng của an-ten hay không.
- **Trả về:** `true` nếu phát hiện có thẻ, `false` nếu không có.
- *Lưu ý:* Nên gọi hàm này liên tục trong vòng lặp chính.

**`bool readCardSerial(uint8_t* serial)`**
- **Chức năng:** Sau khi `checkCard()` trả về `true`, dùng hàm này để trích xuất mã định danh (UID) của thẻ đó.
- **Tham số:** `serial` là mảng (ít nhất 5 byte) để hứng dữ liệu UID trả về.
- **Trả về:** `true` nếu đọc thành công, `false` nếu lỗi (chống xung đột thẻ thất bại).
- *Ví dụ:*
  ```cpp
  uint8_t uid[5];
  if (rfid.checkCard()) {
      if (rfid.readCardSerial(uid)) {
          // uid[0] đến uid[3] chứa 4 byte mã định danh của thẻ
          // uid[4] chứa mã kiểm tra sai số BCC (XOR checksum của 4 byte kia)
      }
  }
  ```

---

## 4. Nhóm Hàm Giao Tiếp Dữ Liệu (MIFARE Classic 1K)

Thẻ MIFARE chia bộ nhớ thành nhiều Sector, mỗi Sector có nhiều Block. Để đọc/ghi một Block, bắt buộc phải qua 3 bước: Chọn thẻ -> Xác thực mật khẩu -> Đọc/Ghi.

**`uint8_t selectTag(uint8_t* serial)`**
- **Chức năng:** Chỉ định kết nối với một thẻ cụ thể (trường hợp có nhiều thẻ đè lên nhau). Bắt buộc phải gọi trước khi muốn xác thực mật khẩu.
- **Tham số:** mảng `serial` (UID) vừa đọc được ở hàm `readCardSerial()`.

**`uint8_t PCD_Authenticate(uint8_t command, uint8_t blockAddr, uint8_t *key, uint8_t *uid)`**
- **Chức năng:** Xác thực mật khẩu để mở khóa vùng nhớ.
- **Tham số:**
  - `command`: Truyền `PICC_CMD_MF_AUTH_KEY_A` hoặc `PICC_CMD_MF_AUTH_KEY_B`.
  - `blockAddr`: Địa chỉ Block muốn thao tác.
  - `key`: Mảng 6 byte mật khẩu (Mặc định thẻ xuất xưởng thường là `{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}`).
  - `uid`: Mảng UID của thẻ.
- **Trả về:** `STATUS_OK` nếu đúng mật khẩu, `STATUS_ERROR` nếu sai.

**`uint8_t MIFARE_Read(uint8_t blockAddr, uint8_t *buffer, uint8_t *bufferSize)`**
- **Chức năng:** Đọc toàn bộ 16 byte dữ liệu từ một Block.
- **Tham số:**
  - `blockAddr`: Địa chỉ Block.
  - `buffer`: Mảng để hứng dữ liệu (chắc chắn phải đủ 18 byte do thư viện yêu cầu tính toán thêm 2 byte mã CRC).
  - `bufferSize`: Kích thước mảng (truyền vào biến có giá trị 18).
- **Trả về:** `STATUS_OK` hoặc `STATUS_ERROR`.

**`uint8_t MIFARE_Write(uint8_t blockAddr, uint8_t *buffer, uint8_t bufferSize)`**
- **Chức năng:** Ghi 16 byte dữ liệu đè lên một Block.
- **Tham số:** 
  - `buffer`: Mảng chứa chính xác 16 byte dữ liệu cần ghi.
  - `bufferSize`: Chiều dài (chắc chắn phải là 16).

---

## 5. Nhóm Hàm Kết Thúc Giao Tiếp

**`void PICC_HaltA()`**
- **Chức năng:** Lệnh đưa thẻ vào trạng thái "Ngủ" (Halt). Dùng khi  đã đọc/ghi xong và không muốn thẻ bị quét lại liên tục khi người dùng vẫn đang áp thẻ trên đầu đọc.

**`void PCD_StopCrypto1()`**
- **Chức năng:** Dừng chế độ mã hóa (Crypto). Bắt buộc phải gọi sau khi thực hiện xong các lệnh `MIFARE_Read` hoặc `MIFARE_Write` để giải phóng module RC522, chuẩn bị cho việc đọc một thẻ mới. Nếu quên gọi hàm này, đầu đọc sẽ bị treo.

---

## 6. Nhóm Hàm Cấp Thấp (Private - Dành cho người phát triển thư viện)

Các hàm này không được gọi trực tiếp từ `main.cpp` mà được dùng ở "hậu trường" (bên trong file `Rfid.cpp`) để xử lý việc giao tiếp phần cứng với chip RC522:

**`csLow()`, `csHigh()`, `rstLow()`, `rstHigh()`**
- **Chức năng:** Điều khiển đóng/mở chip (Chip Select) và điều khiển chân Reset vật lý. `csLow()` kéo chân CS xuống mức 0V để bắt đầu khung truyền SPI, `csHigh()` kéo lên 3.3V để kết thúc khung truyền.

**`writeRegister(uint8_t addr, uint8_t val)` và `readRegister(uint8_t addr)`**
- **Chức năng:** Giao tiếp trực tiếp với thanh ghi của RC522 thông qua SPI. Mỗi thanh ghi quản lý một tính năng phần cứng cụ thể (Ví dụ: thanh ghi cấu hình Anten, thanh ghi báo lỗi...). `writeRegister` là ghi đè cấu hình, còn `readRegister` là đọc trạng thái.

**`setBitMask(uint8_t reg, uint8_t mask)` và `clearBitMask(uint8_t reg, uint8_t mask)`**
- **Chức năng:** Thao tác bit (Bitwise). Dùng để BẬT (`setBitMask`) hoặc TẮT (`clearBitMask`) một vài tính năng cụ thể trong một thanh ghi mà KHÔNG làm thay đổi các tính năng khác đang có sẵn trong thanh ghi đó.

**`antennaOn()`**
- **Chức năng:** Bật 2 chân phát sóng TX1 và TX2 của RC522 để tạo ra từ trường liên tục cấp nguồn cho thẻ. Nếu không bật anten, thẻ sẽ không bao giờ được cấp điện để giao tiếp.

**`resetPCD()`**
- **Chức năng:** Kích hoạt lệnh Soft-Reset (Reset bằng phần mềm), yêu cầu chip RC522 khởi động lại về trạng thái mặc định của nhà sản xuất.

**`toCard(uint8_t command, uint8_t* sendData, uint8_t sendLen, uint8_t* backData, uint32_t* backLen)`**
- **Chức năng:** Đây là **hàm cốt lõi quan trọng nhất**. Nó đóng gói dữ liệu, đẩy vào bộ đệm FIFO của RC522, ra lệnh cho RC522 phát sóng truyền dữ liệu đó đến thẻ (PICC), sau đó chờ thẻ phản hồi lại và lấy dữ liệu trả về từ bộ đệm FIFO.
- Mọi hàm đọc UID, kiểm tra thẻ, hay ghi Block đều phải gọi hàm này để thực thi truyền nhận.

**`calculateCRC(uint8_t *data, uint8_t length, uint8_t *result)`**
- **Chức năng:** Giao nhiệm vụ tính toán mã kiểm tra lỗi (CRC) cho chip RC522 xử lý (Hardware CRC) thay vì vi điều khiển phải tự tính bằng phần mềm. Dùng để bảo mật và chống sai lệch dữ liệu khi đọc/ghi thẻ MIFARE.

---

## 7. Ví dụ Code Tổng Hợp (Đọc thẻ & Check mật khẩu)

```cpp
uint8_t uid[5];
uint8_t defaultKey[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// 1. Quét tìm thẻ
if (rfid.checkCard()) {
    // 2. Lấy mã UID
    if (rfid.readCardSerial(uid)) {
        
        // 3. Chọn thẻ để giao tiếp
        if (rfid.selectTag(uid) > 0) {
            
            // 4. Xác thực mật khẩu (Key A) tại Block số 4
            uint8_t status = rfid.PCD_Authenticate(PICC_CMD_MF_AUTH_KEY_A, 4, defaultKey, uid);
            
            if (status == STATUS_OK) {
                // Đọc dữ liệu
                uint8_t readData[18];
                uint8_t len = 18;
                if (rfid.MIFARE_Read(4, readData, &len) == STATUS_OK) {
                    // Xử lý dữ liệu đọc được ở đây...
                }
            }
            
            // 5. Giải phóng module sau khi xong việc
            rfid.PICC_HaltA();
            rfid.PCD_StopCrypto1();
        }
    }
}
```
