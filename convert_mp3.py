import sys
import os
import re

def convert(input_file):
    base_name = os.path.basename(input_file)
    name_no_ext = os.path.splitext(base_name)[0]
    
    # Chuẩn hóa tên (bỏ ký tự đặc biệt, biến thành camelCase hoặc giữ nguyên dạng an toàn)
    safe_name = re.sub(r'\W|^(?=\d)', '_', name_no_ext)
    
    output_file = f"Core/Inc/{safe_name}.h"
    array_name = f"{safe_name}Array"
    length_name = f"{safe_name}Length"
    macro_name = f"{safe_name.upper()}_H"
    
    if not os.path.exists(input_file):
        print(f"Lỗi: Không tìm thấy file {input_file}")
        return

    with open(input_file, "rb") as f:
        data = f.read()

    print(f"Kích thước file: {len(data)} bytes ({len(data)/1024:.2f} KB)")
    
    # Cảnh báo nếu file quá lớn (Vượt quá dung lượng trống của Flash)
    if len(data) > 40000:
        print("CẢNH BÁO: File MP3 khá lớn (> 40KB). Có thể sẽ gây lỗi hết bộ nhớ Flash (Memory Overflow) khi biên dịch code STM32!")
        print("Mẹo: Hãy dùng phần mềm giảm bitrate của file MP3 xuống khoảng 8kbps hoặc 16kbps Mono.")

    with open(output_file, "w") as f:
        f.write(f"#ifndef {macro_name}\n")
        f.write(f"#define {macro_name}\n\n")
        f.write("#include <stdint.h>\n\n")
        f.write(f"const uint32_t {length_name} = {len(data)};\n")
        f.write(f"const uint8_t {array_name}[] = {{\n")
        
        for i, b in enumerate(data):
            if i % 12 == 0:
                f.write("    ")
            f.write(f"0x{b:02X}, ")
            if i % 12 == 11:
                f.write("\n")
                
        f.write("\n};\n\n")
        f.write(f"#endif // {macro_name}\n")

    print(f"Đã tạo file {output_file} thành công! Tên mảng là: {array_name}")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Cách dùng: python convert_mp3.py <ten_file_cua_ban.mp3>")
    else:
        convert(sys.argv[1])
