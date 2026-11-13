Name:           fcitx5-vnkey
Version:        1.0.0
Release:        1
Summary:        Vietnamese input method for fcitx5
License:        GPL-3.0-or-later
URL:            https://github.com/kieuhienvbhp-glitch/vnkey
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  cmake
BuildRequires:  kf6-extra-cmake-modules
BuildRequires:  gcc-c++
BuildRequires:  glibc-devel
BuildRequires:  fcitx5-devel
BuildRequires:  libinput-devel
BuildRequires:  libevdev-devel
BuildRequires:  libnotify-devel
BuildRequires:  dbus-1-devel
BuildRequires:  libxcb-devel
BuildRequires:  systemd-devel
BuildRequires:  wayland-devel

BuildRequires:  sysuser-tools
BuildRequires:  python3
Requires(pre):  sysuser-shadow >= 3.1

%{?systemd_ordering}
Requires:       fcitx5
Requires:       python3-QtPy
Requires:       (python3-PyQt6 or python3-pyside6)
Requires:       python3-dbus-python
Requires:       acl

%description
Vietnamese input method for fcitx5

%prep
%setup -q
find . -type f -name '*.py' -exec sed -i '1s|^#!.*env python3|#!/usr/bin/python3|' {} +

%build
%cmake -DVNKEY_BYTECOMPILE_PYTHON:BOOL=OFF -DBUILD_TESTING:BOOL=ON
%cmake_build
cd %{_builddir}/%{name}-%{version}
%sysusers_generate_pre build/misc/user-vnkey.conf vnkey vnkey.conf

%install
%cmake_install
%find_lang %{name}
%py3_compile %{buildroot}%{_datadir}/fcitx5-vnkey

%check
%ctest

%files -f %{name}.lang
%{_datadir}/licenses/%{name}/LICENSE

%dir %{_datadir}/licenses/%{name}
%dir %{_modulesloaddir}
%{_bindir}/fcitx5-vnkey-server
%{_bindir}/fcitx5-vnkey-settings

%{_libdir}/fcitx5/libvnkey.so

%{_modulesloaddir}/fcitx5-vnkey.conf
%{_unitdir}/fcitx5-vnkey-server@.service
%{_sysusersdir}/vnkey.conf
%{_udevrulesdir}/99-vnkey.rules

%{_datadir}/fcitx5/addon/vnkey.conf
%{_datadir}/fcitx5/inputmethod/vnkey.conf

%{_datadir}/fcitx5/vnkey/

%{_datadir}/fcitx5-vnkey/
%{_datadir}/applications/org.fcitx.Fcitx5.Addon.VNKey.Settings.desktop
%{_datadir}/metainfo/org.fcitx.Fcitx5.Addon.VNKey.metainfo.xml

%{_datadir}/icons/hicolor/scalable/apps/*fcitx-vnkey*.svg
%{_datadir}/icons/hicolor/scalable/status/fcitx-vnkey*.svg

%dir %{_datadir}/icons/breeze
%dir %{_datadir}/icons/breeze/status
%dir %{_datadir}/icons/breeze/status/22
%dir %{_datadir}/icons/breeze/status/24
%{_datadir}/icons/breeze/status/*/fcitx-vnkey*.svg

%dir %{_datadir}/icons/breeze-dark
%dir %{_datadir}/icons/breeze-dark/status
%dir %{_datadir}/icons/breeze-dark/status/22
%dir %{_datadir}/icons/breeze-dark/status/24
%{_datadir}/icons/breeze-dark/status/*/fcitx-vnkey*.svg

%pre -f vnkey.pre

%post
%service_add_post fcitx5-vnkey-server@.service

if [ $1 -eq 1 ]; then
    echo "--- Cấu hình VNKey ---"
    echo "Hướng dẫn sau cài đặt:"
    echo "1. Kích hoạt Server cho user của bạn:"
    echo "   sudo systemctl enable --now fcitx5-vnkey-server@\$(whoami).service"
    echo ""
    echo "2. Cấu hình Fcitx5:"
    echo "   - Mở 'Fcitx5 Configuration', thêm bộ gõ VNKey"
    echo ""
    echo "3. Lưu ý cho Wayland (KDE):"
    echo "   - Hãy chọn 'Fcitx 5' trong phần Virtual Keyboard của hệ thống."
    echo "------------------------------------------------"
elif [ $1 -eq 2 ]; then
    echo "--- Cấu hình VNKey ---"
    echo "Hướng dẫn sau cập nhật:"
    echo "1. Khởi động lại Server cho user của bạn:"
    echo "   sudo systemctl restart fcitx5-vnkey-server@\$(whoami).service"
    echo ""
    echo "2. Cấu hình Fcitx5:"
    echo "   - Mở 'Fcitx5 Configuration', nhấn restart để khởi động lại."
fi

%preun
%service_del_preun fcitx5-vnkey-server@.service

%postun
%service_del_postun fcitx5-vnkey-server@.service

%changelog
* Fri Nov 13 2026 Neo Vn <vnkey@localhost> - 1.0.0-1
- Phiên bản đầu tiên của VNKey.
