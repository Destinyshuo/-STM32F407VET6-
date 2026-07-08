from typing import Callable, Optional

from PyQt5.QtCore import Qt
from PyQt5.QtGui import QFont
from PyQt5.QtWidgets import (
    QComboBox,
    QGridLayout,
    QHBoxLayout,
    QLabel,
    QLineEdit,
    QTextEdit,
    QVBoxLayout,
    QWidget,
)

from widgets import PillButton, ToggleActionButton


class SerialMatrixPanel(QWidget):
    def __init__(self, send_callback: Optional[Callable[[str], None]] = None, parent=None) -> None:
        super().__init__(parent)
        self.send_callback = send_callback
        self._build_ui()

    def _build_ui(self) -> None:
        root = QVBoxLayout(self)
        root.setContentsMargins(20, 18, 20, 20)
        root.setSpacing(16)

        title = QLabel("自动化执行矩阵")
        title.setFont(QFont("Microsoft YaHei UI", 16, QFont.Bold))
        title.setStyleSheet("color: #0a2d1d;")
        root.addWidget(title)

        serial_row = QHBoxLayout()
        serial_row.setSpacing(12)
        serial_label = QLabel("串口选择")
        serial_label.setFont(QFont("Microsoft YaHei UI", 11))
        self.port_combo = self._make_combo(["COM4", "COM3", "COM5", "COM6"])
        self.connect_button = PillButton("连接")
        self.connect_button.setMaximumWidth(92)
        self.disconnect_button = PillButton("断开连接", accent="#eef5f1")
        self.disconnect_button.setStyleSheet(
            """
            QPushButton {
                background: #eef5f1;
                color: #2f6e4b;
                border: 1px solid #d7e7de;
                border-radius: 16px;
                padding: 8px 16px;
            }
            QPushButton:hover { background: #e8f1ec; }
            """
        )
        serial_row.addWidget(serial_label)
        serial_row.addWidget(self.port_combo, 1)
        serial_row.addWidget(self.connect_button)
        serial_row.addWidget(self.disconnect_button)
        root.addLayout(serial_row)

        self.status_label = QLabel("串口状态：未连接")
        self.status_label.setStyleSheet("color: #7a8f7b; font-size: 10pt;")
        root.addWidget(self.status_label)

        settings = QGridLayout()
        settings.setHorizontalSpacing(14)
        settings.setVerticalSpacing(10)
        self.baud_combo = self._make_combo(["115200", "9600", "57600"])
        self.stop_combo = self._make_combo(["1", "1.5", "2"])
        self.data_combo = self._make_combo(["8", "7", "6"])
        self.parity_combo = self._make_combo(["None", "Odd", "Even"])
        labels = [
            ("波特率", self.baud_combo, 0, 0),
            ("停止位", self.stop_combo, 0, 2),
            ("数据位", self.data_combo, 1, 0),
            ("校验位", self.parity_combo, 1, 2),
        ]
        for text, widget, row, col in labels:
            label = QLabel(text)
            label.setFont(QFont("Microsoft YaHei UI", 11, QFont.Bold))
            settings.addWidget(label, row, col)
            settings.addWidget(widget, row, col + 1)
        root.addLayout(settings)

        guard_row = QHBoxLayout()
        guard_row.addWidget(QLabel("守护类型"))
        self.guard_combo = self._make_combo(
            [
                "🌵 耐旱植物（10-30%）",
                "🌿 常规植物（30-50%）",
                "💧 喜湿/水培植物（50-75%）",
            ]
        )
        guard_row.addWidget(self.guard_combo, 1)
        root.addLayout(guard_row)

        self.auto_fill_button = ToggleActionButton("开启自动寻光")
        self.smart_pump_button = ToggleActionButton("智能水泵", "智能水泵（监控中）")
        self.fill_light_button = ToggleActionButton("自动光照补偿")
        self.emergency_button = PillButton("急停", danger=True)

        root.addWidget(self.auto_fill_button)
        pair = QHBoxLayout()
        pair.setSpacing(10)
        pair.addWidget(self.smart_pump_button)
        pair.addWidget(self.fill_light_button)
        root.addLayout(pair)
        root.addWidget(self.emergency_button)

        self.auto_fill_button.toggled.connect(lambda checked: self._send_hex("A1 01" if checked else "A1 00"))
        self.smart_pump_button.toggled.connect(lambda checked: self._send_hex("B1 01" if checked else "B1 00"))
        self.fill_light_button.toggled.connect(lambda checked: self._send_hex("C1 01" if checked else "C1 00"))
        self.emergency_button.clicked.connect(lambda: self._send_hex("FF 00"))

    def _make_combo(self, items) -> QComboBox:
        combo = QComboBox()
        combo.addItems(items)
        combo.setMinimumHeight(36)
        combo.setFont(QFont("Microsoft YaHei UI", 10))
        combo.setStyleSheet(
            """
            QComboBox {
                background: #f3f8f4;
                border: 1px solid #d7e7de;
                border-radius: 10px;
                padding: 6px 10px;
                color: #0f3b2b;
            }
            QComboBox::drop-down { border: none; width: 24px; }
            """
        )
        return combo

    def _send_hex(self, payload: str) -> None:
        if self.send_callback:
            self.send_callback(payload)


class AiCommandPanel(QWidget):
    def __init__(self, send_callback: Optional[Callable[[str], None]] = None, parent=None) -> None:
        super().__init__(parent)
        self.send_callback = send_callback
        self._build_ui()

    def _build_ui(self) -> None:
        root = QVBoxLayout(self)
        root.setContentsMargins(20, 18, 20, 20)
        root.setSpacing(14)

        title = QLabel("AI 交互指挥舱")
        title.setFont(QFont("Microsoft YaHei UI", 16, QFont.Bold))
        title.setStyleSheet("color: #0a2d1d;")
        root.addWidget(title)

        self.log_box = QTextEdit()
        self.log_box.setReadOnly(True)
        self.log_box.setMinimumHeight(216)
        self.log_box.setStyleSheet(
            """
            QTextEdit {
                background: #f8fbf9;
                border: 1px solid #d7e7de;
                border-radius: 14px;
                padding: 8px;
                color: #0a2d1d;
                font-size: 10.5pt;
            }
            """
        )
        root.addWidget(self.log_box)

        send_row = QHBoxLayout()
        send_row.setSpacing(8)
        self.input_edit = QLineEdit()
        self.input_edit.setPlaceholderText("在此输入文字指令...")
        self.input_edit.setMinimumHeight(44)
        self.input_edit.setStyleSheet(
            """
            QLineEdit {
                background: #ffffff;
                border: 1px solid #d7e7de;
                border-radius: 16px;
                padding: 0 18px;
                color: #173826;
                font-size: 10.5pt;
            }
            """
        )
        self.send_button = PillButton("发送")
        self.send_button.setMaximumWidth(80)
        self.send_button.clicked.connect(self._send_manual_text)
        send_row.addWidget(self.input_edit, 1)
        send_row.addWidget(self.send_button)
        root.addLayout(send_row)

    def append_normal(self, text: str) -> None:
        self.log_box.append(text)

    def append_report(self, text: str) -> None:
        self.log_box.append(text)

    def append_hex(self, text: str) -> None:
        self.log_box.append(f'<span style="color:#2fbe5d;">{text}</span>')

    def _send_manual_text(self) -> None:
        text = self.input_edit.text().strip()
        if not text:
            return
        if self.send_callback:
            self.send_callback(text)
        self.input_edit.clear()
