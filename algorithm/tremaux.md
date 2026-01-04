<img width="400" height="340" alt="image" src="https://github.com/user-attachments/assets/eb0f27bb-250b-4a0a-af6d-cf09cbcb7e06" />
<img width="515" height="500" alt="image" src="https://github.com/user-attachments/assets/3243646b-020e-44d5-a6d9-4e621c17708b" />

# Mô tả chi tiết giải thuật lập trình (Maze Solving Robot)

## 1) Ý tưởng chung
Robot di chuyển theo các **cảm biến hợp lệ**:
- `cb0`: Thẳng
- `cb1`: Trái
- `cb2`: Phải

Quy tắc ưu tiên cơ bản: **Thẳng → Trái → Phải**, nhưng có xét thêm điều kiện **ô đã đi qua** (đánh dấu 0/1/2) để ưu tiên đi vào ô chưa đi lần nào.

Quy ước đánh dấu ô:
- `0`: chưa đi qua
- `1`: đã đi qua 1 lần
- `2`: đường cụt / đã quay đầu / đi lại

---

## 2) Bảng mô tả từng bước theo tọa độ

| Tọa độ | Hướng robot hiện tại | Cảm biến hợp lệ | Thực thi lệnh |
|---|---|---|---|
| (5,2) | BẮC | Trái (cb1) | Gọi hàm xoay trái 90° và đi thẳng. Đánh dấu ô (5,2) từ 0 → 1. |
| (5,1) | TÂY | Phải (cb2) | Xoay phải 90° và đi thẳng 1 ô. Đánh dấu ô = 1. |
| (4,1) | BẮC | Phải (cb2) | Xoay phải 90° và đi thẳng. Đánh dấu ô = 1. |
| (4,2) | ĐÔNG | Trái (cb1) | Xoay trái 90° và đi thẳng. Đánh dấu ô = 1. |
| (3,2) | BẮC | Trái (cb1) | Xoay trái 90° và đi thẳng. Đánh dấu ô = 1. |
| (3,1) | TÂY | Thẳng (cb0), Phải (cb2) | Ưu tiên **thẳng–trái–phải**. Thẳng→(3,0), phải→(2,1). Cả hai ô đều 0 nên ưu tiên **đi thẳng**. |
| (3,0) | TÂY | Trái (cb1) | Xoay trái 90° và đi thẳng. Đánh dấu ô = 1. |
| (5,0) | NAM | ĐƯỜNG CỤT | Xoay 180° và đi thẳng (quay đầu). Đánh dấu ô = 2. |
| (4,0) | BẮC | Thẳng (cb0) | Đi thẳng. Đánh dấu ô = 2. |
| (3,0) | BẮC | Phải (cb2) | Xoay phải và đi thẳng. Đánh dấu ô = 2. |
| (3,1) | ĐÔNG | Thẳng (cb0), Trái (cb1) | Thẳng và trái đều trống, nhưng thẳng→(3,2) đã = 1, còn trái→(2,1) chưa đi. ⇒ Xoay trái và đi thẳng. Đánh dấu (3,1) = 2. |
| (2,1) | BẮC | Phải (cb2) | Xoay phải và đi thẳng. Đánh dấu ô = 1. |
| (2,2) | ĐÔNG | Thẳng (cb0), Trái (cb1) | Ưu tiên đi thẳng vì 2 ô đều chưa đi qua lần nào. |
| (2,3) | ĐÔNG | Phải (cb2) | Xoay phải và đi thẳng. Đánh dấu ô = 1. |
| (3,3) | NAM | Thẳng (cb0), Trái (cb1) | Ưu tiên đi thẳng. Đánh dấu ô = 1. |
| (4,3) | NAM | Thẳng (cb0) | Chạy thẳng. Đánh dấu ô = 1. |
| (5,3) | NAM | ĐƯỜNG CỤT | Xoay 180° và đi thẳng. Đánh dấu ô = 2. |
| (4,3) | BẮC | Thẳng (cb0) | Đi thẳng. Đánh dấu ô = 2. |
| (3,3) | BẮC | Thẳng (cb0), Phải (cb2) | Thẳng không ưu tiên vì đã đi 1 lần, còn phải chưa đi ⇒ rẽ phải đi hướng mới. |
| (3,4) | ĐÔNG | Trái (cb1), Phải (cb2) | Hai ô đều 0 ⇒ ưu tiên rẽ trái. |
| (2,4) | BẮC | Thẳng (cb0) | Đi thẳng. Đánh dấu ô = 1. |
| (1,4) | BẮC | Thẳng (cb0), Trái (cb1) | Ưu tiên đi thẳng. Đánh dấu ô = 1. |
| (0,4) | BẮC | ĐƯỜNG CỤT | Xoay 180° và đánh dấu ô = 2. |
| (1,4) | NAM | Thẳng (cb0), Phải (cb2) | Thẳng→(2,4) đã đi 1 lần ⇒ loại. Xoay phải 90° và đi thẳng. Đánh dấu ô = 1. |
| (1,3) | TÂY | Thẳng (cb0) | Chạy thẳng. Đánh dấu ô = 1. |
| (1,2) | TÂY | Thẳng (cb0), Trái (cb1) | Thẳng→(1,1) chưa đi ⇒ chạy thẳng. |
| (1,1) | TÂY | Thẳng (cb0), Phải (cb2) | Ưu tiên chạy thẳng. Đánh dấu ô = 1. |
| (1,0) | TÂY | Trái (cb1) | Xoay trái 90° và đi thẳng. Đánh dấu ô = 1. |
| (2,0) | NAM | ĐƯỜNG CỤT | Xoay 180° và chạy thẳng. Đánh dấu ô = 2. |
| (1,0) | BẮC | Phải (cb2) | Xoay phải 90° và chạy thẳng. Đánh dấu ô = 2. |
| (1,1) | ĐÔNG | Thẳng (cb0), Trái (cb1) | Thẳng là ô đã đi 1 lần ⇒ ưu tiên rẽ trái và đi thẳng. |
| (0,1) | BẮC | (Đến đích) | Xét điều kiện đã đến đích ⇒ xe dừng chạy. |
