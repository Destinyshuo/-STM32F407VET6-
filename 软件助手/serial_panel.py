from typing import Callable, Optional

from PyQt5.QtCore import QDateTime, QPoint, Qt, QTimer, pyqtSignal
from PyQt5.QtGui import QColor, QFont, QPainter, QPen, QPolygon
from PyQt5.QtWidgets import (
    QComboBox,
    QGridLayout,
    QHBoxLayout,
    QLabel,
    QLineEdit,
    QSizePolicy,
    QTextEdit,
    QPushButton,
    QVBoxLayout,
    QWidget,
)

from widgets import PillButton, ToggleActionButton

try:
    import serial
    from serial.tools import list_ports
except ImportError:
    serial = None
    list_ports = None


def available_serial_ports() -> list:
    if list_ports is None:
        return []
    return [port.device for port in list_ports.comports()]


class TriangleComboBox(QComboBox):
    def paintEvent(self, event) -> None:
        super().paintEvent(event)
        painter = QPainter(self)
        painter.setRenderHint(QPainter.Antialiasing)
        right = self.rect().right()
        center_y = self.rect().center().y()
        painter.setPen(Qt.NoPen)
        painter.setBrush(QColor("#2f463a"))
        triangle = QPolygon(
            [
                QPoint(right - 20, center_y - 2),
                QPoint(right - 10, center_y - 2),
                QPoint(right - 15, center_y + 5),
            ]
        )
        painter.drawPolygon(triangle)


class TrashButton(QPushButton):
    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        self.setToolTip("清空 AI 交互舱")
        self.setCursor(Qt.PointingHandCursor)
        self.setFixedSize(30, 30)
        self.setStyleSheet(
            """
            QPushButton {
                background: #f3f8f4;
                border: 1px solid #d7e7de;
                border-radius: 15px;
            }
            QPushButton:hover { background: #e8f1ec; }
            QPushButton:pressed { background: #dff1e8; }
            """
        )

    def paintEvent(self, event) -> None:
        super().paintEvent(event)
        painter = QPainter(self)
        painter.setRenderHint(QPainter.Antialiasing)
        pen = QPen(QColor("#2f6e4b"), 1.7)
        pen.setCapStyle(Qt.RoundCap)
        pen.setJoinStyle(Qt.RoundJoin)
        painter.setPen(pen)
        painter.setBrush(Qt.NoBrush)

        painter.drawLine(10, 11, 20, 11)
        painter.drawLine(13, 8, 17, 8)
        painter.drawLine(12, 11, 13, 22)
        painter.drawLine(18, 11, 17, 22)
        painter.drawLine(13, 22, 17, 22)
        painter.drawLine(14, 14, 14, 19)
        painter.drawLine(16, 14, 16, 19)


class SerialMatrixPanel(QWidget):
    sensor_data_received = pyqtSignal(dict)
    plant_type_changed = pyqtSignal(int, str)
    pump_manual_toggled = pyqtSignal(bool)

    def __init__(self, send_callback: Optional[Callable[[str], None]] = None, parent=None) -> None:
        super().__init__(parent)
        self.send_callback = send_callback
        self.serial_connection = None
        self._plant_type_options = [
            (0x01, "耐旱"),
            (0x02, "常规"),
            (0x03, "喜湿/水培"),
        ]
        self._auto_seek_mode = 0
        self._receive_buffer = bytearray()
        self.receive_timer = QTimer(self)
        self.receive_timer.setInterval(5)
        self.receive_timer.timeout.connect(self._read_serial_data)
        self._known_ports = []
        self.port_monitor_timer = QTimer(self)
        self.port_monitor_timer.setInterval(2000)
        self.port_monitor_timer.timeout.connect(self._poll_serial_ports)
        self._build_ui()

    def _build_ui(self) -> None:
        root = QVBoxLayout(self)
        root.setContentsMargins(24, 18, 24, 22)
        root.setSpacing(12)

        title = QLabel("自动化执行矩阵")
        title.setFont(QFont("Microsoft YaHei UI", 13, QFont.Bold))
        title.setStyleSheet("background: transparent; border: none; color: #0a2d1d;")
        root.addWidget(title)

        serial_row = QHBoxLayout()
        serial_row.setSpacing(12)
        serial_label = QLabel("串口选择")
        serial_label.setFont(QFont("Microsoft YaHei UI", 11))
        self.port_combo = self._make_combo(["无可用串口"])
        self.port_combo.setEnabled(False)
        self.connect_button = PillButton("连接")
        self.connect_button.setMaximumWidth(84)
        self.disconnect_button = PillButton("断开连接", accent="#eef5f1")
        serial_row.addWidget(serial_label)
        serial_row.addWidget(self.port_combo, 1)
        serial_row.addWidget(self.connect_button)
        serial_row.addWidget(self.disconnect_button)
        root.addLayout(serial_row)

        self.status_label = QLabel("串口状态：未连接")
        self.status_label.setStyleSheet("background: transparent; border: none; color: #7a8f7b; font-size: 9pt;")
        root.addWidget(self.status_label)

        settings = QGridLayout()
        settings.setHorizontalSpacing(18)
        settings.setVerticalSpacing(14)
        settings.setContentsMargins(0, 4, 0, 8)
        self.baud_combo = self._make_combo(["1200", "2400", "4800", "9600", "19200", "38400", "57600", "115200", "230400", "460800", "921600"])
        self.baud_combo.setCurrentText("115200")
        self.stop_combo = self._make_combo(["1", "1.5", "2"])
        self.data_combo = self._make_combo(["5", "6", "7", "8"])
        self.data_combo.setCurrentText("8")
        self.parity_combo = self._make_combo(["None", "Odd", "Even", "Mark", "Space"])
        labels = [
            ("波特率", self.baud_combo, 0, 0),
            ("停止位", self.stop_combo, 0, 2),
            ("数据位", self.data_combo, 1, 0),
            ("校验位", self.parity_combo, 1, 2),
        ]
        for text, widget, row, col in labels:
            label = QLabel(text)
            label.setFont(QFont("Microsoft YaHei UI", 10, QFont.Bold))
            settings.addWidget(label, row, col)
            settings.addWidget(widget, row, col + 1)
        settings.setColumnStretch(1, 1)
        settings.setColumnStretch(3, 1)
        root.addLayout(settings)

        guard_row = QHBoxLayout()
        guard_row.setSpacing(10)
        guard_row.setContentsMargins(0, 4, 0, 0)
        guard_row.addWidget(QLabel("守护类型"))
        self.guard_combo = self._make_combo(
            [
                "耐旱植物（5-25%）",
                "常规植物（25-40%）",
                "喜湿/水培植物（40-75%）",
            ]
        )
        self.guard_combo.setMinimumWidth(190)
        guard_row.addWidget(self.guard_combo, 1)
        root.addLayout(guard_row)

        self.auto_fill_button = ToggleActionButton("开启自动寻光", "开启自动寻光")
        self.smart_pump_button = ToggleActionButton("开启水泵", "水泵运行中")
        self.fill_light_button = ToggleActionButton("开启补光", "补光运行中")
        self.emergency_button = ToggleActionButton("急停", "急停中")

        root.addWidget(self.auto_fill_button)
        pair = QHBoxLayout()
        pair.setSpacing(10)
        pair.addWidget(self.smart_pump_button)
        pair.addWidget(self.fill_light_button)
        root.addLayout(pair)
        root.addWidget(self.emergency_button)

        self.auto_fill_button.clicked.connect(self._cycle_auto_seek_button)
        self.smart_pump_button.toggled.connect(self._handle_manual_pump_toggle)
        self.fill_light_button.toggled.connect(lambda checked: self.set_fill_light_state(checked, send=True))
        self.emergency_button.toggled.connect(lambda checked: self.set_emergency_state(checked, send=True))
        self.guard_combo.currentIndexChanged.connect(self._send_selected_plant_type)
        self.connect_button.clicked.connect(self.connect_serial)
        self.disconnect_button.clicked.connect(self.disconnect_serial)
        self._update_connection_button_styles(False)

    def _frame(self, command: int, value: int) -> str:
        return f"AA {command:02X} {value:02X} 55"

    def set_auto_seek_state(self, enabled: bool, send: bool = True) -> None:
        self._auto_seek_mode = 1 if enabled else 0
        self._sync_auto_seek_button()
        if send:
            self._send_hex(self._frame(0x10, 0x02 if enabled else 0x01))

    def _cycle_auto_seek_button(self) -> None:
        self._auto_seek_mode = 1 if self._auto_seek_mode != 1 else 2
        self._sync_auto_seek_button()
        self._send_hex(self._frame(0x10, 0x02 if self._auto_seek_mode == 1 else 0x01))

    def _sync_auto_seek_button(self) -> None:
        text = "开启自动寻光" if self._auto_seek_mode != 2 else "检测植物状态"
        self.auto_fill_button.blockSignals(True)
        self.auto_fill_button.setChecked(self._auto_seek_mode != 0)
        self.auto_fill_button.blockSignals(False)
        self.auto_fill_button.apply_visual_state(self._auto_seek_mode != 0, text)

    def set_pump_state(self, enabled: bool, send: bool = True) -> None:
        self._set_button_checked(self.smart_pump_button, enabled)
        if send:
            self._send_hex(self._frame(0x20, 0x01 if enabled else 0x00))

    def _handle_manual_pump_toggle(self, checked: bool) -> None:
        self.pump_manual_toggled.emit(checked)
        self.set_pump_state(checked, send=True)

    def set_fill_light_state(self, enabled: bool, send: bool = True) -> None:
        self._set_button_checked(self.fill_light_button, enabled)
        if send:
            self._send_hex(self._frame(0x30, 0x01 if enabled else 0x00))

    def set_emergency_state(self, enabled: bool, send: bool = True) -> None:
        self._set_button_checked(self.emergency_button, enabled)
        if send:
            self._send_hex(self._frame(0x40, 0x01 if enabled else 0x00))

    def send_timestamp(self) -> bool:
        now = QDateTime.currentDateTime()
        frame = [
            0xAA,
            0x60,
            now.date().year() % 100,
            now.date().month(),
            now.date().day(),
            now.time().hour(),
            now.time().minute(),
            now.time().second(),
            0x55,
        ]
        return self.send_payload(" ".join(f"{value:02X}" for value in frame))

    def _send_selected_plant_type(self, index: int) -> None:
        if index < 0 or index >= len(self._plant_type_options):
            return
        code, text = self._plant_type_options[index]
        self._send_hex(self._frame(0x70, code))
        self.plant_type_changed.emit(code, text)

    def _sync_device_state_after_connect(self) -> None:
        current_index = self.guard_combo.currentIndex()
        if 0 <= current_index < len(self._plant_type_options):
            code, _ = self._plant_type_options[current_index]
            self.send_payload(self._frame(0x70, code))

        self.send_payload(self._frame(0x10, 0x02 if self._auto_seek_mode == 1 else 0x01))
        self.send_payload(self._frame(0x20, 0x01 if self.smart_pump_button.isChecked() else 0x00))
        self.send_payload(self._frame(0x30, 0x01 if self.fill_light_button.isChecked() else 0x00))
        self.send_payload(self._frame(0x40, 0x01 if self.emergency_button.isChecked() else 0x00))

    def _set_button_checked(self, button: ToggleActionButton, checked: bool) -> None:
        if button.isChecked() == checked:
            return
        button.blockSignals(True)
        button.setChecked(checked)
        button.blockSignals(False)
        button._sync_style(checked)

    def refresh_available_ports(self) -> None:
        ports = available_serial_ports()
        self._known_ports = ports
        self._sync_port_combo(ports)

    def start_port_monitor(self) -> None:
        self.refresh_available_ports()
        if not self.port_monitor_timer.isActive():
            self.port_monitor_timer.start()

    def _poll_serial_ports(self) -> None:
        ports = available_serial_ports()
        if ports == self._known_ports:
            return

        self._known_ports = ports
        if self.is_connected():
            connected_port = self.serial_connection.port
            if connected_port not in ports:
                try:
                    self.serial_connection.close()
                finally:
                    self.serial_connection = None
                    self.receive_timer.stop()
                    self._receive_buffer.clear()
                    self._set_status(f"串口状态：{connected_port} 已移除", "#f05c47")
                    self.port_combo.setEnabled(True)
                    self.connect_button.setEnabled(True)
                    self._sync_port_combo(ports)
            return

        self._sync_port_combo(ports)

    def _sync_port_combo(self, ports) -> None:
        current_text = self.port_combo.currentText()
        self.port_combo.blockSignals(True)
        self.port_combo.clear()
        if ports:
            self.port_combo.addItems(ports)
            if current_text in ports:
                self.port_combo.setCurrentText(current_text)
            self.port_combo.setEnabled(True)
        else:
            self.port_combo.addItem("无可用串口")
            self.port_combo.setEnabled(False)
        self.port_combo.blockSignals(False)

    def is_connected(self) -> bool:
        return bool(self.serial_connection and self.serial_connection.is_open)

    def connect_serial(self) -> None:
        if serial is None:
            self._set_status("串口状态：pyserial 未安装", "#f05c47")
            return
        if self.is_connected():
            self._set_status(f"串口状态：已连接 {self.serial_connection.port}", "#2dbb5a")
            self._update_connection_button_styles(True)
            return

        port = self.port_combo.currentText().strip()
        if not port or port == "无可用串口":
            self._set_status("串口状态：无可用串口", "#f05c47")
            return

        try:
            self.serial_connection = serial.Serial(
                port=port,
                baudrate=int(self.baud_combo.currentText()),
                bytesize=int(self.data_combo.currentText()),
                parity=self._parity_value(self.parity_combo.currentText()),
                stopbits=self._stop_bits_value(self.stop_combo.currentText()),
                timeout=0,
                write_timeout=0.5,
            )
            self.serial_connection.reset_input_buffer()
        except Exception as exc:
            self.serial_connection = None
            self._set_status(f"串口状态：连接失败（{exc}）", "#f05c47")
            return

        self._receive_buffer.clear()
        self.receive_timer.start()
        self._set_status(f"串口状态：已连接 {port}", "#2dbb5a")
        self.port_combo.setEnabled(False)
        self.connect_button.setEnabled(False)
        self._update_connection_button_styles(True)
        self._sync_device_state_after_connect()

    def disconnect_serial(self) -> None:
        if not self.is_connected():
            self.serial_connection = None
            self.receive_timer.stop()
            self._receive_buffer.clear()
            self._set_status("串口状态：未连接", "#7a8f7b")
            self.port_combo.setEnabled(self.port_combo.currentText() != "无可用串口")
            self.connect_button.setEnabled(True)
            self._update_connection_button_styles(False)
            return
        port = self.serial_connection.port
        try:
            self.serial_connection.close()
        finally:
            self.serial_connection = None
            self.receive_timer.stop()
            self._receive_buffer.clear()
            self._set_status(f"串口状态：已断开 {port}", "#7a8f7b")
            self.port_combo.setEnabled(self.port_combo.currentText() != "无可用串口")
            self.connect_button.setEnabled(True)
            self._update_connection_button_styles(False)

    def _update_connection_button_styles(self, connected: bool) -> None:
        self.connect_button.setStyleSheet(self._connection_button_style(active=connected, primary=True))
        self.disconnect_button.setStyleSheet(self._connection_button_style(active=not connected, primary=False))

    def _connection_button_style(self, active: bool, primary: bool) -> str:
        if active:
            bg = "#35be63"
            fg = "#ffffff"
            border = "#35be63"
            hover = "#2dad58"
        elif primary:
            bg = "#eef5f1"
            fg = "#2f6e4b"
            border = "#d7e7de"
            hover = "#e8f1ec"
        else:
            bg = "#eef5f1"
            fg = "#2f6e4b"
            border = "#d7e7de"
            hover = "#e8f1ec"

        return f"""
            QPushButton {{
                background: {bg};
                color: {fg};
                border: 1px solid {border};
                border-radius: 14px;
                padding: 8px 16px;
            }}
            QPushButton:hover {{ background: {hover}; }}
            QPushButton:disabled {{
                background: {bg};
                color: {fg};
                border: 1px solid {border};
            }}
        """

    def _read_serial_data(self) -> None:
        if not self.is_connected():
            self.receive_timer.stop()
            return

        try:
            waiting = self.serial_connection.in_waiting
            if waiting <= 0:
                return
            self._receive_buffer.extend(self.serial_connection.read(waiting))
        except Exception as exc:
            port = getattr(self.serial_connection, "port", "")
            self.disconnect_serial()
            self._set_status(f"串口状态：接收失败（{port} {exc}）", "#f05c47")
            return

        self._parse_receive_buffer()

    def _parse_receive_buffer(self) -> None:
        while len(self._receive_buffer) >= 4:
            header_index = self._receive_buffer.find(bytes([0xAA]))
            if header_index < 0:
                self._receive_buffer.clear()
                return
            if header_index:
                del self._receive_buffer[:header_index]
            if len(self._receive_buffer) < 4:
                return

            frame_length = self._expected_frame_length(self._receive_buffer)
            if frame_length is None:
                del self._receive_buffer[0]
                continue
            if len(self._receive_buffer) < frame_length:
                return
            if self._receive_buffer[frame_length - 1] != 0x55:
                del self._receive_buffer[0]
                continue

            frame = bytes(self._receive_buffer[:frame_length])
            frame_type = frame[1]
            if frame_type == 0x10:
                self._handle_sensor_frame(frame)
            elif frame_type == 0x20:
                self._handle_obstacle_frame(frame)
            else:
                self._handle_status_frame(frame)
            del self._receive_buffer[:frame_length]

    def _expected_frame_length(self, buffer: bytearray) -> Optional[int]:
        if len(buffer) < 2 or buffer[0] != 0xAA:
            return None

        frame_type = buffer[1]
        if frame_type == 0x10:
            if len(buffer) < 3:
                return None
            sensor_type = buffer[2]
            if sensor_type in (0x10, 0x11, 0x12, 0x13):
                return 6
            return None
        if frame_type == 0x20:
            return 7
        if frame_type in (0x30, 0x70):
            return 4
        return None

    def _handle_sensor_frame(self, frame: bytes) -> None:
        sub_type = frame[2]
        raw_value = (frame[3] << 8) | frame[4]
        payload = {
            0x10: ("soil_moisture", raw_value / 10.0),
            0x11: ("temperature", raw_value / 10.0),
            0x12: ("air_humidity", raw_value / 10.0),
            0x13: ("air_quality", raw_value),
        }.get(sub_type)
        if payload is None:
            return
        name, value = payload
        self.sensor_data_received.emit({"type": name, "value": value, "frame": frame.hex(" ").upper()})

    def _handle_obstacle_frame(self, frame: bytes) -> None:
        yaw_raw = (frame[2] << 8) | frame[3]
        distance = (frame[4] << 8) | frame[5]
        yaw_angle = yaw_raw / 32768.0 * 180.0
        self.sensor_data_received.emit(
            {
                "type": "obstacle",
                "angle": yaw_angle,
                "distance": distance,
                "frame": frame.hex(" ").upper(),
            }
        )

    def _handle_status_frame(self, frame: bytes) -> None:
        frame_type = frame[1]
        code = frame[2]
        if frame_type == 0x30:
            text = {0x00: "健康", 0x01: "黄叶", 0x02: "腐烂", 0x03: "未识别"}.get(code, "未识别")
            self.sensor_data_received.emit(
                {"type": "leaf_status", "code": code, "text": text, "frame": frame.hex(" ").upper()}
            )
            return
        if frame_type == 0x70:
            text = {0x01: "耐旱", 0x02: "常规", 0x03: "喜湿/水培"}.get(code, "未知")
            self.sensor_data_received.emit(
                {"type": "plant_type", "code": code, "text": text, "frame": frame.hex(" ").upper()}
            )

    def _set_status(self, text: str, color: str) -> None:
        self.status_label.setText(text)
        self.status_label.setStyleSheet(
            f"background: transparent; border: none; color: {color}; font-size: 9pt;"
        )

    def _parity_value(self, text: str):
        return {
            "None": serial.PARITY_NONE,
            "Odd": serial.PARITY_ODD,
            "Even": serial.PARITY_EVEN,
            "Mark": serial.PARITY_MARK,
            "Space": serial.PARITY_SPACE,
        }.get(text, serial.PARITY_NONE)

    def _stop_bits_value(self, text: str):
        return {
            "1": serial.STOPBITS_ONE,
            "1.5": serial.STOPBITS_ONE_POINT_FIVE,
            "2": serial.STOPBITS_TWO,
        }.get(text, serial.STOPBITS_ONE)

    def _make_combo(self, items) -> QComboBox:
        combo = TriangleComboBox()
        combo.addItems(items)
        combo.setMinimumHeight(42)
        combo.setMinimumWidth(118)
        combo.setFont(QFont("Microsoft YaHei UI", 10))
        combo.setStyleSheet(
            """
            QComboBox {
                background: #f3f8f4;
                border: 1px solid #d7e7de;
                border-radius: 10px;
                padding: 4px 14px;
                padding-right: 34px;
                color: #0f3b2b;
            }
            QComboBox::drop-down {
                subcontrol-origin: padding;
                subcontrol-position: top right;
                width: 28px;
                border: none;
            }
            QComboBox::down-arrow {
                image: none;
                width: 10px;
                height: 8px;
                margin-right: 10px;
            }
            """
        )
        return combo

    def _send_hex(self, payload: str) -> None:
        if self.send_callback:
            self.send_callback(payload)

    def send_payload(self, payload: str) -> bool:
        if not self.is_connected():
            return False

        text = payload.strip()
        try:
            data = bytes.fromhex(text)
        except ValueError:
            data = text.encode("utf-8")

        try:
            self.serial_connection.write(data)
            return True
        except Exception as exc:
            self._set_status(f"串口状态：发送失败（{exc}）", "#f05c47")
            return False


class AiCommandPanel(QWidget):
    def __init__(self, send_callback: Optional[Callable[[str], None]] = None, parent=None) -> None:
        super().__init__(parent)
        self.send_callback = send_callback
        self._build_ui()

    def _build_ui(self) -> None:
        root = QVBoxLayout(self)
        root.setContentsMargins(22, 16, 22, 20)
        root.setSpacing(12)

        header = QHBoxLayout()
        header.setSpacing(8)
        title = QLabel("AI 交互指挥舱")
        title.setFont(QFont("Microsoft YaHei UI", 13, QFont.Bold))
        title.setStyleSheet("background: transparent; border: none; color: #0a2d1d;")
        self.clear_button = TrashButton()
        self.clear_button.clicked.connect(self.clear_log)
        header.addWidget(title)
        header.addStretch(1)
        header.addWidget(self.clear_button)
        root.addLayout(header)

        self.log_box = QTextEdit()
        self.log_box.setReadOnly(True)
        self.log_box.setMinimumHeight(132)
        self.log_box.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
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
        root.addWidget(self.log_box, 1)

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
        self.send_button.setMinimumHeight(44)
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

    def clear_log(self) -> None:
        self.log_box.clear()

    def _send_manual_text(self) -> None:
        text = self.input_edit.text().strip()
        if not text:
            return
        if self.send_callback:
            self.send_callback(text)
        self.input_edit.clear()
