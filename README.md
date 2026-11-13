[English](README.en.md) | [Tiếng Việt](README.md)

<a id="readme-top"></a>

<div align="center">
  <img src="data/fcitx-vnkey-README.svg" alt="Logo" width="80" height="80">

<h2 align="center">Fcitx5 VNKey</h2>

<p align="center">
    <b>Bộ gõ tiếng Việt đơn giản, hiệu năng cao cho Linux</b>
  </p>
</div>

<br />

Dự án này là bản fork được tối ưu hóa từ [bộ gõ VMK](https://github.com/thanhpy2009/VMK). Chân thành cảm ơn tác giả Thành đã đặt nền móng cho bộ gõ này.

Được phát triển bởi **Neo Vn**, Telegram: [@neovn123](https://t.me/neovn123).

Mã nguồn và bản cài: <https://github.com/kieuhienvbhp-glitch/vnkey> · [Tải bản mới nhất](https://github.com/kieuhienvbhp-glitch/vnkey/releases/latest) · [Báo lỗi](https://github.com/kieuhienvbhp-glitch/vnkey/issues)

## Cài đặt & Hướng dẫn sử dụng

Trên Deepin/Debian, đóng gói và cài bằng:

```bash
./build-deb.sh
sudo dpkg -i fcitx5-vnkey_*_amd64.deb && fcitx5 -r -d
```

Sau khi cài, mở **VNKey Settings** để chọn kiểu gõ, chế độ gõ và luật cho từng ứng dụng.

---

## Biên dịch từ mã nguồn (Dành cho nhà phát triển)

Nếu bạn muốn tự biên dịch bộ gõ từ mã nguồn để đóng góp hoặc tuỳ chỉnh:

### Yêu cầu hệ thống

- **Debian/Ubuntu:** `sudo apt-get install cmake extra-cmake-modules libdbus-1-dev libfcitx5core-dev libfcitx5config-dev libfcitx5utils-dev libevdev-dev libinput-dev libnotify-dev libudev-dev libxcb1-dev libxcb-xtest0-dev libwayland-dev libwayland-bin g++ hicolor-icon-theme pkg-config fcitx5-modules-dev python3-qtpy python3-dbus acl`
- **Fedora/RHEL:** `sudo dnf install cmake dbus-devel extra-cmake-modules fcitx5-devel libevdev-devel libinput-devel libnotify-devel libudev-devel libxcb-devel gcc-c++ hicolor-icon-theme systemd-devel python3-pyside6 python3-QtPy python3-dbus acl wayland-devel`
- **openSUSE:** `sudo zypper install cmake dbus-1-devel kf6-extra-cmake-modules fcitx5-devel libevdev-devel libinput-devel libnotify-devel libxcb-devel systemd-devel gcc-c++ hicolor-icon-theme systemd-devel python3-pyside6 python3-QtPy python3-dbus-python acl wayland-devel`
- **Arch:** `sudo pacman -S --needed cmake extra-cmake-modules gcc git python make pkgconf acl fcitx5 libevdev libinput libnotify libxcb hicolor-icon-theme python-qtpy python-dbus wayland`

### Cài đặt

```bash
cd fcitx5-vnkey
cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_INSTALL_LIBDIR=/usr/lib . #LIBDIR tuỳ vào distro
make
sudo make install
```

---

<a id="đóng-góp"></a>

## 🤝 Đóng góp

Đóng góp là điều làm cho cộng đồng mã nguồn mở trở thành một nơi tuyệt vời để học hỏi, truyền cảm hứng và sáng tạo. Mọi đóng góp của bạn đều được **đánh giá cao**.

Vui lòng xem hướng dẫn chi tiết [tại đây](CONTRIBUTING.md) để biết cách tham gia phát triển dự án, quy trình Pull Request, quy tắc code style và **quy tắc ứng xử**.

---

<a id="giấy-phép"></a>

## 📃 Giấy phép

Dự án được phân phối dưới giấy phép **GNU General Public License v3 (GPL-3.0-or-later)**. Xem [`LICENSE`](LICENSE) để biết thêm chi tiết.

Các thư viện bên thứ ba (bao gồm các thành phần theo giấy phép MIT) vẫn giữ nguyên bản quyền của các tác giả gốc. Cụ thể:

- **[bamboo-core](https://github.com/BambooEngine/bamboo-core)** — engine gõ tiếng Việt gốc bằng Go, MIT, © 2018 Luong Thanh Lam. Bản MIT được giữ tại [`vnkey/LICENSE.bamboo-core`](vnkey/LICENSE.bamboo-core).
- **[VMK](https://github.com/thanhpy2009/VMK)** — dự án mà addon này fork trực tiếp, GPL-3.0-or-later.
- **[fcitx5-bamboo](https://github.com/fcitx/fcitx5-bamboo)** — addon fcitx5 mà phần glue được dựa theo, LGPL-2.1.

Phần C++ của engine (`vnkey/`) là bản port từ bamboo-core và phát hành dưới GPL-3.0-or-later. Danh sách ghi công đầy đủ có trong [`CREDITS`](CREDITS) và trong mục **About** của ứng dụng.
