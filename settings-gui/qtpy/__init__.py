# Lớp tương thích tối thiểu thay cho gói "qtpy" (Deepin không có python3-qtpy):
# trỏ thẳng về PyQt5 có sẵn trên hệ thống.
API = "pyqt5"
API_NAME = "PyQt5"
PYQT5 = True
PYQT6 = PYSIDE2 = PYSIDE6 = False
from PyQt5.QtCore import QT_VERSION_STR as QT_VERSION  # noqa: E402,F401
