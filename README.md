# Maze-Solving Robot (ESP32)

## 1. Tổng quan
Dự án xây dựng một robot tự hành có khả năng **tự di chuyển trong mê cung theo dạng lưới**, sử dụng cảm biến để nhận biết tường/vật cản và thuật toán để quyết định hướng đi.  
Hệ thống được thiết kế theo hướng **vừa vận hành thực tế vừa có khả năng giám sát dữ liệu**, nhằm phục vụ kiểm thử thuật toán và đánh giá chất lượng điều khiển trong môi trường mê cung.

Mục tiêu của đề tài là:
- Robot có thể **tự khám phá mê cung** và xử lý được các tình huống như ngã rẽ, vòng lặp, ngõ cụt.
- **Ghi nhận dữ liệu** (khoảng cách cảm biến, trạng thái di chuyển) để quan sát và phân tích trên dashboard.
- Ổn định chuyển động nhờ phản hồi encoder, giảm sai lệch do cơ khí và tải.

## 2. Kiến trúc hệ thống
Hệ thống gồm 3 lớp chính:

### (1) Lớp cảm biến – nhận biết môi trường
Robot sử dụng **03 cảm biến siêu âm HC-SR04** bố trí theo 3 hướng **trước / trái / phải** để đo khoảng cách đến tường.  
Tại mỗi “bước” trong mê cung, robot cập nhật khoảng cách và phân loại đường đi: có thể đi thẳng, có thể rẽ, hoặc bị chặn.

### (2) Lớp điều khiển chuyển động – chấp hành
Robot dùng **02 động cơ DC có encoder** điều khiển qua **L298N**. Encoder được dùng để:
- cân bằng tốc độ hai bánh khi đi thẳng,
- hỗ trợ điều khiển rẽ/quay ổn định hơn,
- hạn chế sai số tích lũy do lệch cơ khí.

Robot có LCD 16x2 (I2C) hiển thị thông tin cơ bản trong quá trình chạy.

### (3) Lớp truyền thông – giám sát
Robot kết nối Wi-Fi và **truyền dữ liệu qua MQTT** (publish/subscribe). Dữ liệu được hiển thị bằng **Node-RED dashboard** để theo dõi realtime và hỗ trợ đánh giá quá trình chạy.

Nguồn cấp dùng pin 18650 cho phần công suất; khối điều khiển dùng hạ áp **LM2596** để ổn định cho ESP32 và ngoại vi.

## 3. Nguyên lý hoạt động (luồng thực thi)
Luồng chạy được tổ chức theo vòng lặp:
1. **Khởi tạo** ESP32, thiết lập GPIO/Timer, khởi tạo LCD và (tuỳ chọn) kết nối Wi-Fi + MQTT.
2. **Đọc cảm biến** (trước/trái/phải) để xác định các hướng có thể đi.
3. **Ra quyết định hướng đi** dựa trên luật ưu tiên và trạng thái đường đã đi (mục 4).
4. **Điều khiển động cơ**: phát PWM qua L298N; dùng encoder để cân bằng hai bánh và giảm lệch hướng.
5. **Cập nhật trạng thái**: lưu dấu đường đã đi trong ma trận biểu diễn mê cung, đồng thời gửi dữ liệu lên MQTT/Node-RED.
6. Lặp lại cho đến khi **tới đích** hoặc người dùng dừng.

Điểm quan trọng: robot di chuyển theo từng “ô” trong lưới mê cung; mỗi lần hoàn thành 1 ô thì mới cập nhật cảm biến và quyết định bước tiếp theo.

## 4. Thuật toán tìm đường và chiến lược tránh lặp
Robot áp dụng **thuật toán Trémaux** để xử lý mê cung có vòng lặp/ngõ cụt.  
Ý tưởng chính là **đánh dấu số lần đi qua mỗi ô/nhánh** (ví dụ: 0 – chưa đi, 1 – đã đi, 2 – hạn chế đi lại), từ đó:
- ưu tiên các hướng chưa đi qua,
- nếu gặp ngõ cụt thì quay lui,
- giảm khả năng chạy vòng lặp vô hạn.

Thuật toán được lưu bằng mảng/ma trận 2D đại diện lưới mê cung để robot có thể “nhớ” trạng thái đường đi.

## 5. Mô hình mê cung & thử nghiệm
Mê cung được thiết kế theo dạng lưới ô vuông để robot dễ quy ước di chuyển.  
Trong thử nghiệm:
- cảm biến siêu âm đáp ứng ổn định ở khoảng cách làm việc,
- động cơ hoạt động tốt với nguồn thực tế (có dòng khởi động lớn hơn dòng ổn định),
- việc dùng encoder giúp robot cân bằng tốt hơn, hạn chế lệch khi đi thẳng,
- Node-RED dashboard hiển thị dữ liệu theo thời gian thực, thuận tiện quan sát quá trình robot ra quyết định.

## 6. Cấu trúc thư mục repo
- `firmware/` : chương trình ESP32 (đọc cảm biến, điều khiển động cơ, encoder, MQTT)
- `hardware/` : sơ đồ đấu nối, mạch nguồn/driver, hình ảnh phần cứng
- `docs/` : mô tả hệ thống, lưu đồ, hình minh họa
- `results/` : hình ảnh, log, video minh họa kết quả chạy
- `algorithm` : Chi tiết thuật toán hoạt động thực tế

