# SPDX-FileCopyrightText: 2026 Nguyen Hoang Ky <nhktmdzhg@gmail.com>
#
# SPDX-License-Identifier: GPL-3.0-or-later

from i18n import _
from qtpy.QtCore import Qt
from qtpy.QtGui import QIcon
from qtpy.QtWidgets import (
    QFrame,
    QLabel,
    QScrollArea,
    QVBoxLayout,
    QWidget,
)

from ui.components import ScrollContent

try:
    from version import __version__
except ImportError:
    __version__ = "dev version"  # Fallback for local development


class AboutPage(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        self._setup_ui()

    def _setup_ui(self):
        # Root layout for this widget
        root_layout = QVBoxLayout(self)
        root_layout.setContentsMargins(0, 0, 0, 0)

        # Scroll Area to handle overcrowding
        scroll = QScrollArea()
        scroll.setWidgetResizable(True)
        scroll.setFrameShape(QFrame.NoFrame)
        scroll.setHorizontalScrollBarPolicy(Qt.ScrollBarAlwaysOff)
        scroll.setAttribute(Qt.WA_TranslucentBackground)

        content_widget = ScrollContent()
        content_widget.setObjectName("AboutContent")

        layout = QVBoxLayout(content_widget)
        layout.setContentsMargins(30, 18, 30, 24)
        layout.setSpacing(20)
        layout.setAlignment(Qt.AlignTop | Qt.AlignHCenter)

        # Logo/Icon
        try:
            pixmap = QIcon.fromTheme("fcitx-vnkey").pixmap(80, 80)
            if pixmap.isNull():
                logo = QLabel("V")
                logo.setStyleSheet("font-size: 64px; margin-bottom: 5px;")
            else:
                logo = QLabel()
                logo.setPixmap(pixmap)
                logo.setStyleSheet("margin-bottom: 5px;")
        except Exception:
            logo = QLabel("V")
            logo.setStyleSheet("font-size: 64px; margin-bottom: 5px;")

        layout.addWidget(logo, alignment=Qt.AlignCenter)

        title = QLabel("Fcitx5 VNKey")
        title.setObjectName("AboutTitle")
        layout.addWidget(title, alignment=Qt.AlignCenter)

        version = QLabel(_("Version {}").format(__version__))
        version.setObjectName("VersionTag")
        version.setAlignment(Qt.AlignCenter)
        version.setStyleSheet("""
            QLabel#VersionTag {
                background-color: palette(highlight);
                color: palette(highlighted-text);
                border-radius: 10px;
                padding: 2px 10px;
                font-size: 11px;
                font-weight: bold;
            }
        """)
        layout.addWidget(version, alignment=Qt.AlignCenter)

        desc = QLabel(_("Modern, fast, and stable Vietnamese input method for Linux."))
        desc.setWordWrap(True)
        desc.setAlignment(Qt.AlignCenter)
        desc.setObjectName("AboutDescription")
        desc.setMinimumHeight(60)
        layout.addWidget(desc, alignment=Qt.AlignCenter)

        line = QFrame()
        line.setFrameShape(QFrame.HLine)
        line.setObjectName("AboutLine")
        layout.addWidget(line)

        # Credits Section
        credits_title = QLabel(_("Developed by"))
        credits_title.setObjectName("CreditsTitle")
        layout.addWidget(credits_title, alignment=Qt.AlignCenter)

        # Tác giả
        author = QLabel("<b>Neo Vn</b>")
        author.setObjectName("AuthorLink")
        author.setTextFormat(Qt.RichText)
        layout.addWidget(author, alignment=Qt.AlignCenter)

        telegram = QLabel(
            'Telegram: <a href="https://t.me/neovn123" style="text-decoration: none;">@neovn123</a>'
        )
        telegram.setTextFormat(Qt.RichText)
        telegram.setOpenExternalLinks(True)
        telegram.setCursor(Qt.PointingHandCursor)
        layout.addWidget(telegram, alignment=Qt.AlignCenter)

        repo = QLabel(
            'GitHub: <a href="https://github.com/kieuhienvbhp-glitch/vnkey" style="text-decoration: none;">kieuhienvbhp-glitch/vnkey</a>'
        )
        repo.setTextFormat(Qt.RichText)
        repo.setOpenExternalLinks(True)
        repo.setCursor(Qt.PointingHandCursor)
        layout.addWidget(repo, alignment=Qt.AlignCenter)

        layout.addStretch()

        # Footer (Open Source Licenses)
        footer_line = QFrame()
        footer_line.setFrameShape(QFrame.HLine)
        footer_line.setObjectName("AboutLine")
        layout.addWidget(footer_line)

        license_title = QLabel(_("Open Source Licenses"))
        license_title.setObjectName("CreditsTitle")
        layout.addWidget(license_title, alignment=Qt.AlignCenter)

        # Main License
        main_license_info = QLabel(
            _(
                "This project is licensed under the <b>GNU General Public License v3.0 or later (GPL-3.0-or-later)</b>."
            )
        )
        main_license_info.setTextFormat(Qt.RichText)
        main_license_info.setWordWrap(True)
        main_license_info.setAlignment(Qt.AlignCenter)
        main_license_info.setObjectName("LicenseInfo")
        layout.addWidget(main_license_info)

        # Third-party & Upstream Info
        third_party_info = QLabel(
            _(
                "Based on upstream work originally licensed under <b>LGPL-2.1-or-later</b>.<br>"
                "Includes third-party components licensed under the <b>MIT License</b>."
            )
        )
        third_party_info.setTextFormat(Qt.RichText)
        third_party_info.setWordWrap(True)
        third_party_info.setAlignment(Qt.AlignCenter)
        third_party_info.setObjectName("LicenseInfo")
        third_party_info.setStyleSheet("opacity: 0.8; margin-top: 5px;")
        layout.addWidget(third_party_info)

        scroll.setWidget(content_widget)
        root_layout.addWidget(scroll)
