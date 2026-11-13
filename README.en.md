[English](README.en.md) | [Tiếng Việt](README.md)

<a id="readme-top"></a>

<div align="center">
  <img src="data/fcitx-vnkey-README.svg" alt="Logo" width="80" height="80">

<h2 align="center">Fcitx5 VNKey</h2>

<p align="center">
    <b>Simple, high-performance Vietnamese input method for Linux</b>
  </p>
</div>

<br />

This project is an optimized fork of [VMK input method](https://github.com/thanhpy2009/VMK). Sincere thanks to the author Thanh for laying the foundation for this input method.

Developed by **Neo Vn**, Telegram: [@neovn123](https://t.me/neovn123).

Source code and packages: <https://github.com/kieuhienvbhp-glitch/vnkey> · [Latest release](https://github.com/kieuhienvbhp-glitch/vnkey/releases/latest) · [Report a bug](https://github.com/kieuhienvbhp-glitch/vnkey/issues)

## Installation & Usage Guide

On Deepin/Debian, build and install the package with:

```bash
./build-deb.sh
sudo dpkg -i fcitx5-vnkey_*_amd64.deb && fcitx5 -r -d
```

After installing, open **VNKey Settings** to choose the input method, typing mode and per-application rules.

---

## Compilation from source (for developers)

If you want to compile the input method from source to contribute or customize:

### Dependencies

- **Debian/Ubuntu:** `sudo apt-get install cmake extra-cmake-modules libdbus-1-dev libfcitx5core-dev libfcitx5config-dev libfcitx5utils-dev libevdev-dev libinput-dev libnotify-dev libudev-dev libxcb1-dev libxcb-xtest0-dev libwayland-dev libwayland-bin g++ hicolor-icon-theme pkg-config fcitx5-modules-dev python3-qtpy python3-dbus acl`
- **Fedora/RHEL:** `sudo dnf install cmake dbus-devel extra-cmake-modules fcitx5-devel libevdev-devel libinput-devel libnotify-devel libudev-devel libxcb-devel gcc-c++ hicolor-icon-theme systemd-devel python3-pyside6 python3-QtPy python3-dbus acl wayland-devel`
- **openSUSE:** `sudo zypper install cmake dbus-1-devel kf6-extra-cmake-modules fcitx5-devel libevdev-devel libinput-devel libnotify-devel libxcb-devel systemd-devel gcc-c++ hicolor-icon-theme systemd-devel python3-pyside6 python3-QtPy python3-dbus-python acl wayland-devel`
- **Arch:** `sudo pacman -S --needed cmake extra-cmake-modules gcc git python make pkgconf acl fcitx5 libevdev libinput libnotify libxcb hicolor-icon-theme python-qtpy python-dbus wayland`

### Install

```bash
cd fcitx5-vnkey
cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_INSTALL_LIBDIR=/usr/lib . #LIBDIR will vary depending on the distro
make
sudo make install
```

---

<a id="contributing"></a>

## 🤝 Contributing

Contributions are what make the open-source community such an amazing place to learn, inspire, and create. Any contribution you make is **greatly appreciated**.

Please see the detailed guide [here](CONTRIBUTING.en.md) for how to participate in project development, the Pull Request process, code style rules, and **code of conduct**.

---

<a id="license"></a>

## 📃 License

This project is distributed under the terms of the **GNU General Public License v3 (GPL-3.0-or-later)**. See [`LICENSE`](LICENSE) for details.

Third-party libraries (including MIT-licensed components) retain their original licenses and copyrights. Full attributions are available in the application's **About** page.
