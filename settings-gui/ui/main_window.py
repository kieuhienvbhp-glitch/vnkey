# SPDX-FileCopyrightText: 2026 Nguyen Hoang Ky <nhktmdzhg@gmail.com>
#
# SPDX-License-Identifier: GPL-3.0-or-later
"""
Main window assembling all configuration tabs with a modern layout.
"""

from core.dbus_handler import VNKeyDBusHandler
from i18n import _
from qtpy.QtCore import QSize, Qt
from qtpy.QtGui import QIcon
from qtpy.QtWidgets import (
    QApplication,
    QFrame,
    QHBoxLayout,
    QLabel,
    QListWidget,
    QListWidgetItem,
    QMainWindow,
    QPushButton,
    QStackedWidget,
    QVBoxLayout,
    QWidget,
)

# Lazy loading pages on demand


class VNKeySettingsWindow(QMainWindow):
    """Main entry window for VNKey Configuration GUI."""

    def __init__(self):
        super().__init__()
        self.setWindowTitle(_("VNKey Settings"))

        self.dbus_handler = VNKeyDBusHandler()

        self._setup_ui()
        self._setup_window_size()
        self._apply_global_styles()
        self.update_reset_button_state()

    def update_reset_button_state(self):
        any_modified_from_default = any(
            (
                hasattr(self.content_stack.widget(i), "is_modified_from_default")
                and self.content_stack.widget(i).is_modified_from_default()
            )
            or (
                hasattr(self.content_stack.widget(i), "is_modified")
                and self.content_stack.widget(i).is_modified()
            )
            for i in range(self.content_stack.count())
        )
        self.btn_reset.setEnabled(any_modified_from_default)

    ACCENT = "#E53935"

    def _apply_global_styles(self):
        a = self.ACCENT
        import os
        assets = os.path.join(os.path.dirname(os.path.abspath(__file__)), "assets").replace(os.sep, "/")
        self.setStyleSheet(f"""
            /* Khung */
            #SidebarPanel {{
                background: palette(window);
                border-right: 1px solid palette(midlight);
            }}
            QStackedWidget#ContentStack, QStackedWidget#ContentStack QScrollArea,
            QStackedWidget#ContentStack QScrollArea > QWidget > QWidget {{
                background: palette(base);
            }}
            #BottomBar {{
                background: palette(window);
                border-top: 1px solid palette(midlight);
            }}

            /* Thương hiệu ở đầu thanh bên */
            QLabel#BrandName {{ font-size: 17px; font-weight: 700; }}
            QLabel#BrandVersion {{ color: palette(placeholder-text); font-size: 11px; }}

            /* Thanh bên */
            QListWidget#Sidebar {{ border: none; background: transparent; outline: none; }}
            QListWidget#Sidebar::item {{
                padding: 4px 10px; margin: 1px 8px; border-radius: 6px;
            }}
            QListWidget#Sidebar::item:selected {{ background: {a}; color: white; }}
            QListWidget#Sidebar::item:hover:!selected {{ background: palette(midlight); }}

            /* Tiêu đề trang & nhóm */
            QLabel#CategoryTitle {{ font-size: 18px; font-weight: 600; }}
            QLabel#GroupHeader {{
                font-size: 12px; font-weight: 700; color: {a};
                margin-top: 4px; padding-left: 2px;
            }}
            QLabel#AboutTitle {{ font-size: 26px; font-weight: 600; }}

            /* Thẻ nhóm cài đặt */
            QFrame#SettingCard {{
                background: palette(window);
                border: 1px solid palette(midlight);
                border-radius: 10px;
            }}
            QFrame#SettingCard QLabel {{ background: transparent; }}

            /* Nút */
            QPushButton {{ padding: 4px 12px; border-radius: 6px; }}
            QPushButton#Primary {{
                background: {a}; color: white; border: 1px solid {a}; font-weight: 600;
            }}
            QPushButton#Primary:hover {{ background: #EF5350; }}
            QPushButton#Primary:disabled {{ background: palette(midlight); color: palette(placeholder-text); border-color: palette(midlight); }}

            QPushButton {{
                background: palette(button); border: 1px solid palette(mid);
            }}
            QPushButton:hover {{ border-color: {a}; }}
            QPushButton:disabled {{ color: palette(placeholder-text); border-color: palette(midlight); }}
            QPushButton:flat {{ border: none; background: transparent; }}

            /* Ô nhập */
            QComboBox, QLineEdit, QSpinBox {{
                min-height: 24px; padding: 2px 8px;
                background: palette(base); border: 1px solid palette(mid); border-radius: 6px;
            }}
            /* Danh sách luôn sổ xuống dưới ô, không nhảy lên để mục đang chọn đè lên ô */
            QComboBox {{ combobox-popup: 0; }}
            QComboBox:focus, QLineEdit:focus, QSpinBox:focus {{ border-color: {a}; }}
            QComboBox::drop-down {{ border: none; width: 22px; }}
            QSpinBox::up-button, QSpinBox::down-button {{ border: none; width: 18px; background: transparent; }}
            QSpinBox::up-arrow {{ image: url({assets}/arrow-up.svg); width: 10px; height: 10px; }}
            QSpinBox::down-arrow {{ image: url({assets}/arrow-down.svg); width: 10px; height: 10px; }}
            QComboBox::down-arrow {{ image: url({assets}/arrow-down.svg); width: 12px; height: 12px; }}
            QComboBox QAbstractItemView {{
                background: palette(window); border: 1px solid palette(mid);
                selection-background-color: {a}; selection-color: white; outline: none;
            }}

            /* Ô đánh dấu */
            QCheckBox {{ spacing: 8px; background: transparent; }}
            QCheckBox::indicator {{
                width: 16px; height: 16px; border-radius: 4px;
                border: 1.5px solid #5d5f6a; background: palette(base);
            }}
            QCheckBox::indicator:hover {{ border-color: {a}; }}
            QCheckBox::indicator:checked {{
                background: {a}; border-color: {a}; image: url({assets}/check.svg);
            }}
            QCheckBox::indicator:disabled {{ border-color: palette(midlight); }}

            /* Bảng & danh sách trong trang */
            QTableWidget, QTableView, QListWidget#AppList {{
                background: palette(base); border: 1px solid palette(midlight); border-radius: 8px;
                gridline-color: palette(midlight);
            }}
            QHeaderView::section {{
                background: palette(window); color: palette(placeholder-text);
                border: none; border-bottom: 1px solid palette(midlight); padding: 5px 8px; font-weight: 600;
            }}

            /* Thanh cuộn mảnh */
            QScrollBar:vertical {{ background: transparent; width: 10px; margin: 2px; }}
            QScrollBar::handle:vertical {{ background: palette(mid); border-radius: 4px; min-height: 30px; }}
            QScrollBar::handle:vertical:hover {{ background: #5d5f6a; }}
            QScrollBar:horizontal {{ background: transparent; height: 10px; margin: 2px; }}
            QScrollBar::handle:horizontal {{ background: palette(mid); border-radius: 4px; min-width: 30px; }}
            QScrollBar::add-line, QScrollBar::sub-line {{ width: 0; height: 0; }}
            QScrollBar::add-page, QScrollBar::sub-page {{ background: transparent; }}

            /* Phím tắt: ô ghi phím trông như ô nhập, phím bên trong là "phím nhỏ" */
            QPushButton#HotkeyButton {{
                background: palette(base); border: 1px solid palette(mid);
                border-radius: 6px; min-height: 26px; padding: 0px 6px;
            }}
            QPushButton#HotkeyButton:hover {{ border-color: {a}; }}
            QPushButton#HotkeyButton:checked {{ border-color: {a}; background: #2a1d1f; }}
            QPushButton#ClearHotkey {{
                background: transparent; border: none; padding: 0px;
                color: palette(placeholder-text); font-size: 13px;
            }}
            QPushButton#ClearHotkey:hover {{ color: {a}; }}
            QLabel#KeyCap {{
                background: #2c2d35; color: palette(text);
                border: 1px solid #44464f; border-bottom: 2px solid #44464f;
                border-radius: 4px; padding: 0px 7px;
                font-weight: 600; font-family: monospace; font-size: 12px;
            }}
            QLabel#HotkeyPlaceholder {{ color: palette(placeholder-text); }}
            QLabel#ShortcutWarning {{
                color: palette(link-visited);
                font-size: 12px;
            }}
        """)

    def _setup_ui(self):
        central_widget = QWidget()
        self.setCentralWidget(central_widget)

        main_v_layout = QVBoxLayout(central_widget)
        main_v_layout.setContentsMargins(0, 0, 0, 0)
        main_v_layout.setSpacing(0)

        main_h_layout = QHBoxLayout()
        main_h_layout.setContentsMargins(0, 0, 0, 0)
        main_h_layout.setSpacing(0)

        sidebar_panel = QFrame()
        sidebar_panel.setObjectName("SidebarPanel")
        sidebar_panel.setFixedWidth(168)
        panel_layout = QVBoxLayout(sidebar_panel)
        panel_layout.setContentsMargins(0, 14, 0, 8)
        panel_layout.setSpacing(12)
        panel_layout.addWidget(self._build_brand())

        self.sidebar = QListWidget()
        self.sidebar.setVerticalScrollBarPolicy(Qt.ScrollBarAsNeeded)
        self.sidebar.setHorizontalScrollBarPolicy(Qt.ScrollBarAlwaysOff)
        self.sidebar.setObjectName("Sidebar")
        self.sidebar.setFrameShape(QFrame.NoFrame)
        panel_layout.addWidget(self.sidebar, 1)

        self.content_stack = QStackedWidget()
        self.content_stack.setObjectName("ContentStack")

        main_h_layout.addWidget(sidebar_panel)
        main_h_layout.addWidget(self.content_stack, 1)

        main_v_layout.addLayout(main_h_layout, 1)

        # Bottom Bar
        self._setup_bottom_bar(main_v_layout)

        # Pages Mapping
        self._setup_pages()

        self.sidebar.currentRowChanged.connect(self._on_sidebar_changed)
        self.sidebar.setCurrentRow(0)

    def _setup_bottom_bar(self, layout):
        container = QFrame()
        container.setObjectName("BottomBar")
        bar_layout = QHBoxLayout(container)
        bar_layout.setContentsMargins(16, 8, 16, 8)
        bar_layout.setSpacing(10)

        bar_layout.addSpacing(152)

        self.btn_reset = QPushButton(_("&Reset"))
        self.btn_reset.clicked.connect(self.on_restore_defaults)
        bar_layout.addWidget(self.btn_reset)

        bar_layout.addStretch()

        self.btn_cancel = QPushButton(_("&Cancel"))
        self.btn_cancel.setEnabled(False)
        self.btn_cancel.clicked.connect(self.on_cancel)
        bar_layout.addWidget(self.btn_cancel)

        self.btn_apply = QPushButton(_("&Apply"))
        self.btn_apply.setEnabled(False)
        self.btn_apply.clicked.connect(lambda: self.on_save_all(quiet=False))
        bar_layout.addWidget(self.btn_apply)

        self.btn_ok = QPushButton(_("&OK"))
        self.btn_ok.setObjectName("Primary")
        self.btn_ok.clicked.connect(self.on_ok)
        bar_layout.addWidget(self.btn_ok)

        layout.addWidget(container)

    def _setup_pages(self):
        def create_general():
            from ui.pages.dynamic_settings import DynamicSettingsPage, SettingsCategory

            return DynamicSettingsPage(self.dbus_handler, category=SettingsCategory.GENERAL)

        def create_typing():
            from ui.pages.dynamic_settings import DynamicSettingsPage, SettingsCategory

            return DynamicSettingsPage(self.dbus_handler, category=SettingsCategory.TYPING)

        def create_applications():
            from ui.pages.mode_manager import ModeManagerPage

            return ModeManagerPage(self.dbus_handler)

        def create_macros():
            from ui.pages.macro_editor import MacroEditorPage

            return MacroEditorPage(self.dbus_handler)

        def create_dict():
            from ui.pages.dict_editor import DictEditorPage

            return DictEditorPage(self.dbus_handler)

        def create_keymap():
            from ui.pages.keymap_editor import KeymapEditorPage

            return KeymapEditorPage(self.dbus_handler)

        def create_shortcuts():
            from ui.pages.dynamic_settings import DynamicSettingsPage, SettingsCategory

            return DynamicSettingsPage(self.dbus_handler, category=SettingsCategory.SHORTCUTS)

        def create_backup():
            from ui.pages.backup import BackupPage

            return BackupPage(self.dbus_handler)

        def create_about():
            from ui.pages.about import AboutPage

            return AboutPage()

        self._add_header(_("Basic"))
        self._add_page(_("General"), "preferences-system", create_general)
        self._add_page(_("Typing"), "input-keyboard", create_typing)
        self._add_page(_("Macros"), "accessories-text-editor", create_macros)

        self._add_header(_("Advanced"))
        self._add_page(_("Applications"), "applications-other", create_applications)
        self._add_page(_("Shortcuts"), "preferences-desktop-keyboard-shortcuts", create_shortcuts)
        self._add_page(_("Dictionary"), "accessories-dictionary", create_dict)
        self._add_page(_("Keymap"), "preferences-desktop-keyboard", create_keymap)

        self._add_header(_("Other"))
        self._add_page(_("Backup"), "document-save-as", create_backup)

        self._add_page(_("About"), "help-about", create_about)

    def on_restore_defaults(self):
        """Resets all settings to their default values."""
        from qtpy.QtWidgets import QMessageBox

        reply = QMessageBox.question(
            self,
            _("Confirm Reset"),
            _("Restore all settings to defaults?"),
            QMessageBox.Yes | QMessageBox.No,
        )
        if reply == QMessageBox.Yes:
            for i in range(self.content_stack.count()):
                page = self.content_stack.widget(i)
                if hasattr(page, "restore_defaults"):
                    page.restore_defaults()
            # After reset, we definitely have "unsaved changes" relative to previous
            self.on_changed()

    def on_changed(self):
        """Enables/disables the apply and cancel buttons based on pending changes."""
        any_modified = any(
            hasattr(self.content_stack.widget(i), "is_modified")
            and self.content_stack.widget(i).is_modified()
            for i in range(self.content_stack.count())
        )
        has_errors = self.has_validation_errors()
        self.btn_apply.setEnabled(any_modified and not has_errors)
        self.btn_cancel.setEnabled(any_modified)
        self.btn_ok.setEnabled(not has_errors)
        self.update_reset_button_state()

    def has_validation_errors(self):
        return any(
            hasattr(self.content_stack.widget(i), "has_validation_errors")
            and self.content_stack.widget(i).has_validation_errors()
            for i in range(self.content_stack.count())
        )

    def validation_message(self):
        messages = []
        for i in range(self.content_stack.count()):
            page = self.content_stack.widget(i)
            if hasattr(page, "validation_message"):
                message = page.validation_message()
                if message:
                    messages.append(message)
        return "\n".join(messages)

    def on_save_all(self, quiet=False):
        """Triggers save on all pages that support it."""
        from qtpy.QtWidgets import QMessageBox

        if self.has_validation_errors():
            QMessageBox.warning(
                self,
                _("Cannot Save"),
                self.validation_message(),
            )
            return False

        for i in range(self.content_stack.count()):
            page = self.content_stack.widget(i)
            if hasattr(page, "save_data"):
                if page.save_data() is False:
                    QMessageBox.critical(
                        self,
                        _("Error"),
                        _("Failed to save settings. Please check if Fcitx5 is running."),
                    )
                    return False

        self.btn_apply.setEnabled(False)
        self.btn_cancel.setEnabled(False)
        self.update_reset_button_state()
        if not quiet:
            QMessageBox.information(self, _("Success"), _("Settings saved."))
        return True

    def on_ok(self):
        if self.on_save_all(quiet=True):
            self.close()

    def on_cancel(self):
        """Discards all unsaved changes by reloading data on all pages."""
        for i in range(self.content_stack.count()):
            page = self.content_stack.widget(i)
            if hasattr(page, "load_data"):
                page.load_data()
            elif hasattr(page, "load_config"):
                page.load_config()

        self.btn_apply.setEnabled(False)
        self.btn_cancel.setEnabled(False)
        self.btn_ok.setEnabled(not self.has_validation_errors())
        self.update_reset_button_state()

    def _on_sidebar_changed(self, index):
        item = self.sidebar.item(index)
        if not item:
            return

        role = item.data(Qt.UserRole)
        if role == "page":
            widget = item.data(Qt.UserRole + 2)
            if widget is None:
                factory = item.data(Qt.UserRole + 1)
                if factory:
                    widget = factory()
                    self.content_stack.addWidget(widget)
                    item.setData(Qt.UserRole + 2, widget)
            if widget:
                self.content_stack.setCurrentWidget(widget)
            self.update_reset_button_state()
        elif role == "header":
            # Don't allow selecting headers, move to next item
            if index + 1 < self.sidebar.count():
                self.sidebar.setCurrentRow(index + 1)

    def _setup_window_size(self):
        screen = QApplication.primaryScreen().availableGeometry()
        w = min(900, screen.width())
        h = min(580, screen.height())
        self.setMinimumSize(820, 500)
        self.resize(w, h)
        self.move((screen.width() - w) // 2, (screen.height() - h) // 2)

    def _build_brand(self):
        brand = QWidget()
        lay = QHBoxLayout(brand)
        lay.setContentsMargins(16, 0, 10, 0)
        lay.setSpacing(10)
        logo = QLabel()
        icon = QIcon.fromTheme("fcitx-vnkey")
        if icon.isNull():
            icon = QIcon("/usr/share/icons/hicolor/scalable/apps/fcitx-vnkey.svg")
        logo.setPixmap(icon.pixmap(30, 30))
        lay.addWidget(logo)
        texts = QVBoxLayout()
        texts.setSpacing(0)
        name = QLabel("VNKey")
        name.setObjectName("BrandName")
        texts.addWidget(name)
        try:
            from version import __version__
        except ImportError:
            __version__ = ""
        ver = QLabel("v" + __version__.split(" ")[0] if __version__ else "")
        ver.setObjectName("BrandVersion")
        texts.addWidget(ver)
        lay.addLayout(texts, 1)
        return brand

    def _add_header(self, title: str):
        item = QListWidgetItem(title.upper())
        item.setFlags(Qt.NoItemFlags)
        item.setData(Qt.UserRole, "header")
        font = item.font()
        font.setPointSizeF(max(font.pointSizeF() * 0.78, 7.0))
        font.setBold(True)
        item.setFont(font)
        item.setForeground(self.palette().placeholderText())
        item.setSizeHint(QSize(0, 24))
        self.sidebar.addItem(item)

    def _add_page(self, title: str, icon_name: str, page_factory):
        item = QListWidgetItem(title)  # không icon minh hoạ
        item.setData(Qt.UserRole, "page")
        item.setData(Qt.UserRole + 1, page_factory)
        item.setData(Qt.UserRole + 2, None)

        self.sidebar.addItem(item)
