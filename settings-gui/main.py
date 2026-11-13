#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Nguyen Hoang Ky <nhktmdzhg@gmail.com>
#
# SPDX-License-Identifier: GPL-3.0-or-later
"""
Application entry point.
"""

import os
import signal
import sys

from i18n import setup_i18n
from qtpy.QtCore import QTimer
from qtpy.QtGui import QIcon
from qtpy.QtWidgets import QApplication
from PyQt5.QtNetwork import QLocalServer, QLocalSocket
from ui.main_window import VNKeySettingsWindow


def apply_dark_theme(app):
    """Giao diện tối dịu mắt, điểm nhấn đỏ theo logo VNKey (áp dụng cho mọi cửa sổ, hộp thoại)."""
    from qtpy.QtGui import QColor, QPalette

    app.setStyle("Fusion")
    colors = {
        QPalette.Window: "#1e1f24",
        QPalette.WindowText: "#e6e7eb",
        QPalette.Base: "#16171b",
        QPalette.AlternateBase: "#23242a",
        QPalette.Text: "#e6e7eb",
        QPalette.Button: "#2a2b31",
        QPalette.ButtonText: "#e6e7eb",
        QPalette.BrightText: "#ffffff",
        QPalette.Light: "#3c3d45",
        QPalette.Midlight: "#30313a",
        QPalette.Mid: "#3a3b44",
        QPalette.Dark: "#0f1013",
        QPalette.Shadow: "#000000",
        QPalette.Highlight: "#E53935",
        QPalette.HighlightedText: "#ffffff",
        QPalette.Link: "#64b5f6",
        QPalette.LinkVisited: "#ef9a9a",
        QPalette.ToolTipBase: "#2a2b31",
        QPalette.ToolTipText: "#e6e7eb",
        QPalette.PlaceholderText: "#8b8d98",
    }
    pal = QPalette()
    for role, value in colors.items():
        pal.setColor(role, QColor(value))
    for role in (QPalette.WindowText, QPalette.Text, QPalette.ButtonText):
        pal.setColor(QPalette.Disabled, role, QColor("#6b6d76"))
    pal.setColor(QPalette.Disabled, QPalette.Button, QColor("#24252b"))
    app.setPalette(pal)


SINGLE_INSTANCE_KEY = f"org.fcitx.Fcitx5.Addon.VNKey.Settings-{os.getuid()}"


def notify_running_instance():
    """Nếu đã có cửa sổ cài đặt đang mở thì báo nó hiện lên và trả về True."""
    sock = QLocalSocket()
    sock.connectToServer(SINGLE_INSTANCE_KEY)
    if not sock.waitForConnected(500):
        return False
    sock.write(b"show")
    sock.flush()
    sock.waitForBytesWritten(500)
    sock.disconnectFromServer()
    return True


def start_instance_server(window):
    """Lắng nghe các lần mở sau; mỗi lần như vậy đưa cửa sổ hiện có lên trước."""
    # Socket cũ có thể còn sót lại nếu lần trước ứng dụng bị tắt đột ngột.
    QLocalServer.removeServer(SINGLE_INSTANCE_KEY)
    server = QLocalServer(window)
    server.setSocketOptions(QLocalServer.UserAccessOption)
    if not server.listen(SINGLE_INSTANCE_KEY):
        return server

    def bring_to_front():
        while server.hasPendingConnections():
            conn = server.nextPendingConnection()
            conn.readyRead.connect(conn.readAll)
            conn.disconnected.connect(conn.deleteLater)
        if window.isMinimized():
            window.showNormal()
        else:
            window.show()
        window.raise_()
        window.activateWindow()

    server.newConnection.connect(bring_to_front)
    return server


def main():
    """Main execution function."""
    setup_i18n()
    app = QApplication(sys.argv)
    app.setDesktopFileName("org.fcitx.Fcitx5.Addon.VNKey.Settings")
    app.setApplicationName("org.fcitx.Fcitx5.Addon.VNKey.Settings")
    signal.signal(signal.SIGINT, signal.SIG_DFL)
    app.setWindowIcon(QIcon.fromTheme("fcitx-vnkey"))

    if notify_running_instance():
        sys.exit(0)

    apply_dark_theme(app)

    timer = QTimer()
    timer.start(500)
    timer.timeout.connect(lambda: None)

    window = VNKeySettingsWindow()
    window.show()
    instance_server = start_instance_server(window)  # noqa: F841 (giữ tham chiếu)

    try:
        sys.exit(app.exec())
    except KeyboardInterrupt:
        app.quit()


if __name__ == "__main__":
    main()
