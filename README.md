<div align="center">
  <img src="data/vnkey-logo.svg" alt="VNKey" width="96" height="96">

  <h1>VNKey</h1>

  <p><b>Bộ gõ tiếng Việt cho Linux, gõ mượt và không gạch chân</b></p>

  <p>
    <a href="https://github.com/kieuhienvbhp-glitch/vnkey/releases/latest"><b>Tải bản mới nhất</b></a>
    ·
    <a href="https://github.com/kieuhienvbhp-glitch/vnkey/issues">Báo lỗi</a>
  </p>
</div>

## Tính năng

- Gõ **Telex**, **VNI**, **VIQR**.
- **Không gạch chân** khi gõ, chế độ **Tự động** chọn cách gõ hợp với từng ứng dụng (Chrome, terminal, ứng dụng Deepin…).
- Tự trả lại chữ gốc khi bạn gõ từ tiếng Anh, ví dụ `windows` không bị thành "windơ".
- Gõ tắt (macro), kiểm tra chính tả, phím tắt chuyển chế độ.
- Trang cài đặt riêng, dễ dùng.

## Cài đặt (Deepin / Debian / Ubuntu)

1. Tải file `.deb` ở trang [Releases](https://github.com/kieuhienvbhp-glitch/vnkey/releases/latest).
2. Mở terminal ở thư mục chứa file vừa tải và chạy:

   ```bash
   sudo dpkg -i fcitx5-vnkey_*.deb && fcitx5 -r -d
   ```

3. Thêm **VNKey** vào danh sách bộ gõ của fcitx5.
4. Mở **VNKey Settings** để chọn kiểu gõ và chế độ gõ.

## Tự build từ mã nguồn

Cài các gói cần thiết (Debian/Deepin):

```bash
sudo apt install cmake extra-cmake-modules g++ pkg-config gettext \
  libfcitx5core-dev libfcitx5config-dev libfcitx5utils-dev fcitx5-modules-dev \
  libdbus-1-dev libevdev-dev libinput-dev libnotify-dev libudev-dev \
  libxcb1-dev libxcb-xtest0-dev libwayland-dev libwayland-bin \
  python3-pyqt5 python3-dbus
```

Rồi build gói `.deb`:

```bash
git clone https://github.com/kieuhienvbhp-glitch/vnkey.git
cd vnkey
./build-deb.sh
```

## Tác giả

Phát triển bởi **Neo Vn**, Telegram: [@neovn123](https://t.me/neovn123).

## Giấy phép

VNKey phát hành theo giấy phép **GPL-3.0-or-later**, xem [`LICENSE`](LICENSE).
Dựa trên mã nguồn mở của [bamboo-core](https://github.com/BambooEngine/bamboo-core) (MIT), [VMK](https://github.com/thanhpy2009/VMK) và [fcitx5-bamboo](https://github.com/fcitx/fcitx5-bamboo). Chi tiết ghi công trong [`CREDITS`](CREDITS).
