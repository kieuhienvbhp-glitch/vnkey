# Đóng góp cho VNKey

Cảm ơn bạn muốn góp sức cho VNKey! Dưới đây là cách báo lỗi, đề xuất tính năng và gửi mã.

## Báo lỗi

Mở [issue mới](https://github.com/kieuhienvbhp-glitch/vnkey/issues/new/choose), chọn **Báo lỗi** và ghi rõ:

- Phiên bản VNKey (xem ở trang **Giới thiệu** trong VNKey Settings).
- Hệ điều hành, môi trường desktop (Deepin, KDE, GNOME…).
- Ứng dụng bị lỗi (Chrome, terminal, VS Code…) và chế độ gõ đang dùng.
- Bạn đã gõ gì, kết quả ra sao, đúng ra phải ra sao.
- Nếu được, đính kèm log từ lệnh `fcitx5-diagnose`.

## Đề xuất tính năng

Mở issue, chọn **Yêu cầu tính năng**. Hãy tìm xem đã có ai đề xuất giống vậy chưa, rồi mô tả tính năng và lý do bạn cần nó.

## Gửi mã (Pull Request)

1. Fork repo và tải bản fork về máy:

   ```bash
   git clone https://github.com/<tài-khoản-của-bạn>/vnkey.git
   cd vnkey
   git checkout -b sua-loi-gi-do
   ```

2. Sửa mã, rồi build và chạy test:

   ```bash
   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
   cmake --build build -j"$(nproc)"
   cd build && ctest
   ```

   Muốn thử trên máy thật thì build gói `.deb` bằng `./build-deb.sh` rồi cài.

3. Định dạng mã trước khi commit:

   ```bash
   clang-format -i src/*.cpp src/*.h      # C++
   ruff check --fix settings-gui && ruff format settings-gui   # Python
   ```

4. Commit với lời nhắn rõ ràng (ví dụ `Sửa lỗi mất chữ khi gõ trên Chrome`), đẩy lên fork và tạo Pull Request vào nhánh `main`.

**Mỗi Pull Request chỉ nên giải quyết một việc.** Nếu sửa giao diện, hãy kèm ảnh chụp màn hình.

## Quy tắc

- Quy tắc viết mã: [CODE_STYLE.md](CODE_STYLE.md).
- Quy tắc ứng xử: [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md).

## Giấy phép

Khi đóng góp, bạn đồng ý mã của mình được phát hành theo giấy phép của dự án (GPL-3.0-or-later).
