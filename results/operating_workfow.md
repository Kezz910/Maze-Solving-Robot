## Nguyên lý hoạt động 

### 1) Tổng quan phần cứng
Robot sử dụng:
- **03 cảm biến siêu âm HC-SR04** để đo khoảng cách trước / trái / phải (quy ước lần lượt: cb0 / cb1 / cb2).
- **02 động cơ DC có encoder (2 kênh)** để đo tốc độ/độ dịch chuyển, kết hợp bánh caster phía trước để hỗ trợ chuyển hướng.
- Nguồn cấp: **3 cell 18650 (3000 mAh/cell)** → qua mạch mở rộng, sử dụng LM2596 tạo nguồn ổn áp cấp cho ESP32.
- Mạch mở rộng dùng 74HC595N để mở rộng GPIO; đồng thời bổ sung phần cách ly/đệm tải (relay, transistor BC547, diode 1N4007, điện trở...) nhằm hạn chế xung đột và bảo vệ ESP32 khi điều khiển tải công suất.

---

### 2) Trình tự khởi động & truyền dữ liệu giám sát
Khi bật công tắc:
1. ESP32 khởi động và kết nối Wi-Fi.
2. Sau khi Wi-Fi OK, robot tiếp tục kết nối MQTT tới broker (server).
3. Khi MQTT kết nối thành công, hệ thống sẵn sàng và tự chạy sau ~5 giây.
4. Trong quá trình chạy, robot **đẩy dữ liệu khoảng cách (cb0/cb1/cb2)** lên hệ thống giám sát (Node-RED) theo từng “ô” trong mê cung.

---

### 3) Mô hình mê cung & quy ước tọa độ
- Mê cung có kích thước **6 × 5** (6 hàng, 5 cột).
- Mỗi ô được gán một tọa độ **(x, y)**.
- Tọa độ **xuất phát: (5, 2)** và tọa độ **đích: (0, 1)**.
- Robot di chuyển **theo từng ô** (đi hết 1 ô rồi mới cập nhật cảm biến và ra quyết định bước tiếp theo).

---

### 4) Điều khiển chuyển động: PID bám hướng / bám tường
Bộ điều khiển **PID** được dùng để ổn định chuyển động và giảm lệch hướng:
- Robot **bắt buộc đi thẳng hết 1 ô** trước khi đọc lại cảm biến và quyết định rẽ/đi thẳng/xoay.
- Khi **hai bên đều có tường**, PID giữ robot chạy cân bằng giữa hành lang.
- Khi **một bên mất tường**, robot chuyển sang **bám tường** theo phía còn lại.
- Khi **không có tường**, robot điều khiển 2 động cơ chạy “đồng đều” để đi thẳng hết 1 ô.

---

### 5) Thuật toán tìm đường: Trémaux + ưu tiên hướng
Robot áp dụng **thuật toán Trémaux** với nguyên tắc:
- **Mỗi ô không đi quá 2 lần**.
- Mỗi ô có “giá trị đánh dấu”:
  - `0`: chưa đi qua
  - `1`: đã đi qua 1 lần
  - `2`: đã đi qua 2 lần

**Luật ra quyết định tại ngã rẽ**
1. Robot quét trạng thái đường đi theo thứ tự ưu tiên hình học: **THẲNG → TRÁI → PHẢI** (ưu tiên hướng nào thông trước).
2. Ở tầng kiểm định Trémaux, robot sẽ ưu tiên:
   - Chọn ô có giá trị **0** (chưa đi qua) trước.
   - Nếu không còn ô giá trị 0 xung quanh, mới xét ô giá trị **1**.
3. Nếu gặp **đường cụt / bị chặn**, robot quay lại ngã rẽ gần nhất và tiếp tục xét các hướng còn khả dụng theo nguyên tắc trên.

> Tóm lại: robot kết hợp **ổn định chuyển động bằng PID** (đi theo từng ô) và **quy tắc Trémaux** (đánh dấu 0/1/2) để tìm đường trong mê cung theo chiến lược ưu tiên nhưng vẫn tránh lặp vô hạn.
