#!/bin/bash
# Đóng gói VNKey thành .deb (cmake build + install vào thư mục tạm + dpkg-deb)
set -e
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VERSION="$(sed -n 's/^project(fcitx5-vnkey VERSION \([0-9.]*\)).*/\1/p' "$ROOT/CMakeLists.txt")-1~deepin1"
STAGE="$ROOT/dist/staging"
OUT="$ROOT/dist/fcitx5-vnkey_${VERSION}_amd64.deb"

echo "==> 1/3 Biên dịch"
cmake -S "$ROOT" -B "$ROOT/build" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_INSTALL_LIBDIR=lib/x86_64-linux-gnu >/dev/null
cmake --build "$ROOT/build" -j"$(nproc)"

echo "==> 2/3 Cài vào thư mục tạm"
rm -rf "$STAGE"
DESTDIR="$STAGE" cmake --install "$ROOT/build" >/dev/null
find "$STAGE" -name __pycache__ -type d -prune -exec rm -rf {} +
strip --strip-unneeded "$STAGE"/usr/lib/x86_64-linux-gnu/fcitx5/libvnkey.so "$STAGE"/usr/bin/fcitx5-vnkey-server
mkdir -p "$STAGE/DEBIAN"
sed "s/@VERSION@/$VERSION/" "$ROOT/packaging/deb/control" > "$STAGE/DEBIAN/control"
install -m 755 "$ROOT/packaging/deb/preinst" "$ROOT/packaging/deb/postinst" "$ROOT/packaging/deb/prerm" "$ROOT/packaging/deb/postrm" "$STAGE/DEBIAN/"
install -D -m 755 "$ROOT/packaging/deb/dcc-wrapper" "$STAGE/usr/libexec/dcc-fcitx5configtool-exec"

echo "==> 3/3 Đóng gói"
dpkg-deb --root-owner-group -b "$STAGE" "$OUT" >/dev/null
cp -f "$OUT" "$ROOT/"
ls -lh "$OUT"
