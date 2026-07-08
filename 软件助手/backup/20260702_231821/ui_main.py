from datetime import datetime, timedelta
from typing import Optional

from PyQt5.QtCore import QDateTime, QTimer, Qt, pyqtSignal
from PyQt5.QtGui import QFont
from PyQt5.QtWidgets import (
    QApplication,
    QDesktopWidget,
    QFrame,
    QHBoxLayout,
    QLabel,
    QLineEdit,
    QMainWindow,
    QMessageBox,
    QStackedWidget,
    QVBoxLayout,
    QWidget,
)

from auth_state import AuthStateManager
from serial_panel import AiCommandPanel, SerialMatrixPanel
from widgets import EyeToggleButton, MoistureCurveWidget, PillButton, PlantAvatarWidget, RadarWidget, SoilMoistureBar


def card_style(radius: int = 22) -> str:
    return (
        f"QFrame#PanelCard, QWidget#PanelCard {{ "
        f"background: #ffffff; border: none; border-radius: {radius}px; }}"
    )


def make_card(radius: int = 22) -> QFrame:
    card = QFrame()
    card.setObjectName("PanelCard")
    card.setStyleSheet(card_style(radius))
    return card


def label_clean_style(extra: str = "") -> str:
    return f"background: transparent; border: none; {extra}"


class PasswordField(QWidget):
    def __init__(self, placeholder: str, parent=None) -> None:
        super().__init__(parent)
        layout = QHBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(0)
        self.line_edit = QLineEdit()
        self.line_edit.setPlaceholderText(placeholder)
        self.line_edit.setEchoMode(QLineEdit.Password)
        self.line_edit.setMinimumHeight(38)
        self.line_edit.setStyleSheet(
            """
            QLineEdit {
                background: #f4faf5;
                border: 1px solid #edf4ef;
                border-right: none;
                border-radius: 10px 0 0 10px;
                padding: 0 14px;
                color: #173826;
                font-size: 10pt;
            }
            """
        )
        self.eye_button = EyeToggleButton()
        self.eye_button.toggled.connect(self._toggle_echo)
        eye_host = QFrame()
        eye_host.setFixedHeight(38)
        eye_host.setStyleSheet(
            """
            QFrame {
                background: #f4faf5;
                border: 1px solid #edf4ef;
                border-left: none;
                border-radius: 0 10px 10px 0;
            }
            """
        )
        eye_layout = QHBoxLayout(eye_host)
        eye_layout.setContentsMargins(0, 0, 8, 0)
        eye_layout.addStretch(1)
        eye_layout.addWidget(self.eye_button)
        layout.addWidget(self.line_edit, 1)
        layout.addWidget(eye_host)

    def _toggle_echo(self, checked: bool) -> None:
        self.line_edit.setEchoMode(QLineEdit.Normal if checked else QLineEdit.Password)

    def text(self) -> str:
        return self.line_edit.text()

    def clear(self) -> None:
        self.line_edit.clear()


class LoginPage(QWidget):
    login_success = pyqtSignal()

    def __init__(self, auth_manager: AuthStateManager, parent=None) -> None:
        super().__init__(parent)
        self.auth_manager = auth_manager
        self._build_ui()

    def _build_ui(self) -> None:
        self.setObjectName("LoginPage")
        self.setStyleSheet("QWidget#LoginPage { background: #eef8f1; }")
        root = QVBoxLayout(self)
        root.setContentsMargins(0, 0, 0, 0)
        root.addStretch(2)

        headline = QLabel("PLANT INTELLIGENCE ASSISTANT")
        headline.setAlignment(Qt.AlignCenter)
        headline.setFont(QFont("Microsoft YaHei UI Light", 29))
        headline.setStyleSheet(label_clean_style("color: #0e2e22; letter-spacing: 0px;"))
        root.addWidget(headline)

        subtitle = QLabel("植物智能助手 · 环境感知与养护控制中枢")
        subtitle.setAlignment(Qt.AlignCenter)
        subtitle.setFont(QFont("Microsoft YaHei UI", 11))
        subtitle.setStyleSheet(label_clean_style("color: #65866f;"))
        root.addWidget(subtitle)
        root.addSpacing(42)

        card = make_card(24)
        card.setFixedWidth(414)
        card_layout = QVBoxLayout(card)
        card_layout.setContentsMargins(28, 22, 28, 22)
        card_layout.setSpacing(10)

        title = QLabel("唤醒系统登录")
        title.setAlignment(Qt.AlignCenter)
        title.setFont(QFont("Microsoft YaHei UI", 15, QFont.Bold))
        title.setStyleSheet(label_clean_style("color: #0d281d;"))
        card_layout.addWidget(title)

        self.phone_edit = QLineEdit()
        self.phone_edit.setPlaceholderText("请输入手机号")
        self.phone_edit.setMinimumHeight(38)
        self.phone_edit.setStyleSheet(self._entry_style())
        card_layout.addWidget(self.phone_edit)

        self.password_edit = PasswordField("请输入密码")
        card_layout.addWidget(self.password_edit)

        self.confirm_edit = PasswordField("确认密码（注册时使用）")
        card_layout.addWidget(self.confirm_edit)

        button_row = QHBoxLayout()
        button_row.setSpacing(10)
        self.login_button = PillButton("登录系统")
        self.register_button = PillButton("注册账号", accent="#f3f8f4")
        self.register_button.setStyleSheet(
            """
            QPushButton {
                background: #f3f8f4;
                color: #47715b;
                border: 1px solid #edf4ef;
                border-radius: 20px;
                padding: 8px 16px;
            }
            QPushButton:hover { background: #edf5ef; }
            """
        )
        self.login_button.clicked.connect(self._login)
        self.register_button.clicked.connect(self._register)
        button_row.addWidget(self.login_button)
        button_row.addWidget(self.register_button)
        card_layout.addLayout(button_row)

        self.tip_label = QLabel("")
        self.tip_label.setAlignment(Qt.AlignCenter)
        self.tip_label.setStyleSheet(label_clean_style("color: #d45c57; font-size: 10pt;"))
        card_layout.addWidget(self.tip_label)

        holder = QHBoxLayout()
        holder.addStretch(1)
        holder.addWidget(card)
        holder.addStretch(1)
        root.addLayout(holder)
        root.addStretch(3)

    def _entry_style(self) -> str:
        return """
            QLineEdit {
                background: #f4faf5;
                border: 1px solid #edf4ef;
                border-radius: 10px;
                padding: 0 14px;
                color: #173826;
                font-size: 10pt;
            }
        """

    def _register(self) -> None:
        phone = self.phone_edit.text().strip()
        password = self.password_edit.text()
        confirm = self.confirm_edit.text()
        if password != confirm:
            self.tip_label.setText("两次密码输入不一致。")
            return
        ok, message = self.auth_manager.register_user(phone, password)
        self.tip_label.setText(message)
        if ok:
            self.login_success.emit()

    def _login(self) -> None:
        phone = self.phone_edit.text().strip()
        password = self.password_edit.text()
        ok, message = self.auth_manager.login_user(phone, password)
        self.tip_label.setText(message)
        if ok:
            self.login_success.emit()


class WakePage(QWidget):
    wake_requested = pyqtSignal()

    def __init__(self, auth_manager: AuthStateManager, parent=None) -> None:
        super().__init__(parent)
        self.auth_manager = auth_manager
        self._build_ui()

    def _build_ui(self) -> None:
        self.setObjectName("WakePage")
        self.setStyleSheet("QWidget#WakePage { background: #eef8f1; }")
        root = QVBoxLayout(self)
        root.setContentsMargins(0, 0, 0, 0)
        root.addStretch(1)

        headline = QLabel("PLANT INTELLIGENCE ASSISTANT")
        headline.setAlignment(Qt.AlignCenter)
        headline.setFont(QFont("Microsoft YaHei UI Light", 32))
        headline.setStyleSheet(label_clean_style("color: #0e2e22;"))
        root.addWidget(headline)

        subtitle = QLabel("植物智能助手 · 环境感知与养护控制中枢")
        subtitle.setAlignment(Qt.AlignCenter)
        subtitle.setFont(QFont("Microsoft YaHei UI", 12))
        subtitle.setStyleSheet(label_clean_style("color: #65866f;"))
        root.addWidget(subtitle)
        root.addSpacing(34)

        self.wake_button = PillButton("唤醒系统")
        self.wake_button.setFixedSize(320, 58)
        self.wake_button.setStyleSheet(
            """
            QPushButton {
                background: #35be63;
                color: #ffffff;
                border: none;
                border-radius: 24px;
                font-weight: 700;
            }
            QPushButton:hover { background: #2fb35b; }
            QPushButton:pressed { background: #2aa654; }
            """
        )
        self.wake_button.clicked.connect(self.wake_requested.emit)
        holder = QHBoxLayout()
        holder.addStretch(1)
        holder.addWidget(self.wake_button)
        holder.addStretch(1)
        root.addSpacing(76)
        root.addLayout(holder)
        root.addStretch(2)


class DashboardPage(QWidget):
    logout_requested = pyqtSignal()

    def __init__(self, auth_manager: AuthStateManager, parent=None) -> None:
        super().__init__(parent)
        self.auth_manager = auth_manager
        self.system_started_at = datetime.now()
        self.current_moisture = 0.0
        self.current_air_ppm = 36
        self.current_temperature = 26.0
        self.current_humidity = 58.0
        self.current_obstacle_angle: Optional[float] = None
        self.current_obstacle_distance: Optional[float] = None
        self._build_ui()
        self._start_timers()
        self._append_initial_logs()
        self._refresh_dashboard()

    def _build_ui(self) -> None:
        self.setObjectName("DashboardPage")
        self.setStyleSheet("QWidget#DashboardPage { background: #eef8f1; }")
        root = QVBoxLayout(self)
        root.setContentsMargins(14, 20, 14, 16)
        root.setSpacing(16)

        top_row = QHBoxLayout()
        top_row.setSpacing(16)
        top_row.addStretch(1)

        self.time_banner = QLabel("00:00:00")
        self.time_banner.setAlignment(Qt.AlignCenter)
        self.time_banner.setMinimumSize(680, 82)
        self.time_banner.setFont(QFont("Consolas", 26, QFont.Bold))
        self.time_banner.setStyleSheet(
            """
            QLabel {
                background: #35be63;
                color: white;
                border-radius: 28px;
            }
            """
        )
        top_row.addWidget(self.time_banner, 0)
        top_row.addStretch(1)

        profile = QFrame()
        profile.setObjectName("ProfileCard")
        profile.setFixedSize(238, 70)
        profile.setStyleSheet("QFrame#ProfileCard { background: #ffffff; border: none; border-radius: 18px; }")
        profile_layout = QHBoxLayout(profile)
        profile_layout.setContentsMargins(14, 9, 14, 9)
        avatar = PlantAvatarWidget()
        profile_layout.addWidget(avatar)

        profile_text = QVBoxLayout()
        profile_text.setSpacing(2)
        self.user_id_label = QLabel("")
        self.user_id_label.setFont(QFont("Microsoft YaHei UI", 9, QFont.Bold))
        self.user_id_label.setStyleSheet(label_clean_style("color: #0b3323;"))
        self.user_state_label = QLabel("已登录")
        self.user_state_label.setStyleSheet(label_clean_style("color: #2dbb5a; font-size: 9pt;"))
        self.logout_label = QLabel('<a href="logout" style="color:#f05c47;text-decoration:none;font-size:9pt;">退出登录</a>')
        self.logout_label.linkActivated.connect(lambda _: self.logout_requested.emit())
        profile_text.addWidget(self.user_id_label)
        profile_text.addWidget(self.user_state_label)
        profile_text.addWidget(self.logout_label)
        profile_layout.addLayout(profile_text)
        top_row.addWidget(profile, 0, Qt.AlignTop)
        root.addLayout(top_row)

        body = QHBoxLayout()
        body.setSpacing(16)

        left_column = QVBoxLayout()
        left_column.setSpacing(16)
        right_column = QVBoxLayout()
        right_column.setSpacing(16)

        left_column.addWidget(self._build_inspection_card())
        self.serial_matrix = SerialMatrixPanel(send_callback=self._handle_send_command)
        self.serial_matrix.setObjectName("PanelCard")
        self.serial_matrix.setAttribute(Qt.WA_StyledBackground, True)
        self.serial_matrix.setStyleSheet(card_style(20))
        self.serial_matrix.setMinimumWidth(430)
        self.serial_matrix.setMinimumHeight(456)
        right_column.addWidget(self.serial_matrix)

        radar_card = self._build_radar_card()
        radar_card.setMinimumHeight(282)
        left_column.addWidget(radar_card)
        self.ai_panel = AiCommandPanel(send_callback=self._handle_send_command)
        self.ai_panel.setObjectName("PanelCard")
        self.ai_panel.setAttribute(Qt.WA_StyledBackground, True)
        self.ai_panel.setStyleSheet(card_style(20))
        self.ai_panel.setMinimumWidth(430)
        self.ai_panel.setMinimumHeight(304)
        right_column.addWidget(self.ai_panel)
        right_column.addStretch(1)

        left_column.addWidget(self._build_curve_card())
        body.addLayout(left_column, 7)
        body.addLayout(right_column, 5)
        root.addLayout(body)

    def _build_card_title(self, text: str) -> QLabel:
        label = QLabel(text)
        label.setFont(QFont("Microsoft YaHei UI", 16, QFont.Bold))
        label.setStyleSheet(label_clean_style("color: #0a2d1d;"))
        return label

    def _build_inspection_card(self) -> QFrame:
        card = make_card(20)
        layout = QVBoxLayout(card)
        layout.setContentsMargins(18, 16, 18, 18)
        layout.setSpacing(14)
        layout.addWidget(self._build_card_title("MaixCAM 植物巡检摘要"))

        monitor = QFrame()
        monitor.setMinimumHeight(140)
        monitor.setStyleSheet("background: #0f2419; border-radius: 20px;")
        monitor_layout = QVBoxLayout(monitor)
        monitor_layout.setContentsMargins(18, 16, 18, 16)
        monitor_layout.addStretch(1)
        self.plant_status_label = QLabel("当前植物状态：未识别到植物")
        self.plant_status_label.setAlignment(Qt.AlignCenter)
        self.plant_status_label.setFont(QFont("Microsoft YaHei UI", 15))
        self.plant_status_label.setStyleSheet(label_clean_style("color: #9fe4b4;"))
        self.inspection_time_label = QLabel("")
        self.inspection_time_label.setAlignment(Qt.AlignCenter)
        self.inspection_time_label.setFont(QFont("Microsoft YaHei UI", 14))
        self.inspection_time_label.setStyleSheet(label_clean_style("color: #9fe4b4;"))
        monitor_layout.addWidget(self.plant_status_label)
        monitor_layout.addWidget(self.inspection_time_label)
        monitor_layout.addStretch(1)
        layout.addWidget(monitor)
        return card

    def _build_radar_card(self) -> QFrame:
        card = make_card(20)
        layout = QVBoxLayout(card)
        layout.setContentsMargins(18, 16, 18, 22)
        layout.setSpacing(12)
        layout.addWidget(self._build_card_title("全景感知雷达"))

        content = QHBoxLayout()
        content.setSpacing(22)
        self.radar_widget = RadarWidget()
        self.radar_widget.setFixedSize(190, 190)
        content.addWidget(self.radar_widget, 0, Qt.AlignTop)

        right = QVBoxLayout()
        right.setSpacing(11)
        self.environment_label = QLabel("")
        self.environment_label.setFont(QFont("Microsoft YaHei UI", 15, QFont.Bold))
        self.obstacle_label = QLabel("")
        self.obstacle_label.setFont(QFont("Microsoft YaHei UI", 13, QFont.Bold))
        self.soil_title_label = QLabel("土壤湿度：")
        self.soil_title_label.setFont(QFont("Microsoft YaHei UI", 13))
        self.moisture_bar = SoilMoistureBar(self.current_moisture)
        self.moisture_value_label = QLabel("")
        self.moisture_value_label.setFont(QFont("Microsoft YaHei UI", 13, QFont.Bold))

        bar_row = QHBoxLayout()
        bar_row.addWidget(self.moisture_bar, 1)
        bar_row.addWidget(self.moisture_value_label)

        metrics1 = QHBoxLayout()
        metrics1.setSpacing(10)
        self.temp_box = self._metric_box("环境温度", "")
        self.humidity_box = self._metric_box("空气湿度", "")
        metrics1.addWidget(self.temp_box)
        metrics1.addWidget(self.humidity_box)

        self.air_quality_box = self._metric_box("空气质量", "")
        self.air_quality_box.setMinimumHeight(46)
        right.addWidget(self.environment_label)
        right.addWidget(self.obstacle_label)
        right.addWidget(self.soil_title_label)
        right.addLayout(bar_row)
        right.addLayout(metrics1)
        right.addWidget(self.air_quality_box)
        right.addStretch(1)
        content.addLayout(right, 1)
        layout.addLayout(content)
        return card

    def _metric_box(self, title: str, value: str) -> QFrame:
        box = QFrame()
        box.setObjectName("MetricBox")
        box.setMinimumHeight(44)
        box.setStyleSheet("QFrame#MetricBox { background: #f5faf7; border: none; border-radius: 16px; }")
        layout = QHBoxLayout(box)
        layout.setContentsMargins(14, 9, 14, 9)
        label = QLabel(title)
        label.setFont(QFont("Microsoft YaHei UI", 11, QFont.Bold))
        value_label = QLabel(value)
        value_label.setObjectName("valueLabel")
        value_label.setFont(QFont("Microsoft YaHei UI", 11, QFont.Bold))
        label.setStyleSheet(label_clean_style("color: #2b7650;"))
        value_label.setStyleSheet(label_clean_style("color: #0b3323;"))
        layout.addWidget(label)
        layout.addStretch(1)
        layout.addWidget(value_label)
        box.value_label = value_label
        return box

    def _build_curve_card(self) -> QFrame:
        card = make_card(20)
        layout = QVBoxLayout(card)
        layout.setContentsMargins(20, 18, 20, 18)
        layout.setSpacing(12)

        header = QHBoxLayout()
        header.addWidget(self._build_card_title("土壤湿度曲线"))
        header.addStretch(1)
        self.curve_current_label = QLabel("")
        self.curve_current_label.setFont(QFont("Microsoft YaHei UI", 12, QFont.Bold))
        self.curve_current_label.setStyleSheet(label_clean_style("color: #0b3323;"))
        header.addWidget(self.curve_current_label)
        layout.addLayout(header)

        self.curve_widget = MoistureCurveWidget()
        layout.addWidget(self.curve_widget)
        return card

    def _start_timers(self) -> None:
        self.clock_timer = QTimer(self)
        self.clock_timer.timeout.connect(self._update_time)
        self.clock_timer.start(1000)
        self._update_time()

    def _update_time(self) -> None:
        now = QDateTime.currentDateTime().toString("HH:mm:ss")
        self.time_banner.setText(now)

    def _append_initial_logs(self) -> None:
        user_id = self.auth_manager.get_active_user() or ""
        self.ai_panel.append_normal("系统：植物智能助手已启动")
        self.ai_panel.append_normal("串口：可手动选择 COM 口并按当前参数连接。")
        self.ai_panel.append_normal("说明：交互舱用于显示日志、收发记录和人工输入指令。")
        self.ai_panel.append_normal(f"系统唤醒成功：{user_id}")

    def _handle_send_command(self, payload: str) -> None:
        self.ai_panel.append_hex(payload)

    def _refresh_dashboard(self) -> None:
        user_id = self.auth_manager.get_active_user() or "未登录"
        self.user_id_label.setText(f"用户ID：{user_id}")

        inspect_time = self.system_started_at - timedelta(minutes=5)
        self.inspection_time_label.setText(f"最近一次巡检时间：{inspect_time.strftime('%Y-%m-%d %H:%M:%S')}")

        self.moisture_bar.setValue(self.current_moisture)
        self.moisture_value_label.setText(f"{self.current_moisture:.1f}%")
        self.temp_box.value_label.setText(f"{self.current_temperature:.1f} ℃")
        self.humidity_box.value_label.setText(f"{self.current_humidity:.1f} %")

        if self.current_air_ppm <= 50:
            self.air_quality_box.value_label.setText(f"正常（{self.current_air_ppm} ppm）")
            self.air_quality_box.value_label.setStyleSheet("color: #28be5f;")
        else:
            self.air_quality_box.value_label.setText(f"污染（{self.current_air_ppm} ppm）")
            self.air_quality_box.value_label.setStyleSheet("color: #f05c47;")

        self._update_radar_status()
        labels = [datetime.now().strftime("%H:%M:%S")] * 3
        self.curve_widget.set_curve_data([self.current_moisture] * 6, labels)
        self.curve_current_label.setText(f"当前值：{self.current_moisture:.1f}%")

    def _update_radar_status(self) -> None:
        angle = self.current_obstacle_angle
        distance = self.current_obstacle_distance
        if distance is None or distance > 200:
            self.environment_label.setText("环境安全，待机中")
            self.environment_label.setStyleSheet("color: #1bc457;")
            self.obstacle_label.setText("障碍角度： --°  障碍距离： -- cm")
            self.obstacle_label.setStyleSheet("color: #1bc457;")
            self.radar_widget.set_obstacle(None, None)
            return
        if distance > 100:
            self.environment_label.setText("环境安全")
            self.environment_label.setStyleSheet("color: #1bc457;")
            self.obstacle_label.setText(f"障碍角度： {angle:.0f}°  障碍距离： {distance:.0f} cm")
            self.obstacle_label.setStyleSheet("color: #1bc457;")
            self.radar_widget.set_obstacle(angle, distance)
            return
        self.environment_label.setText("前方近距离处有障碍物")
        self.environment_label.setStyleSheet("color: #ff5b43;")
        self.obstacle_label.setText(f"障碍角度： {angle:.0f}°  障碍距离： {distance:.0f} cm")
        self.obstacle_label.setStyleSheet("color: #ff5b43;")
        self.radar_widget.set_obstacle(angle, distance)


class PlantAssistantMainWindow(QMainWindow):
    def __init__(self) -> None:
        super().__init__()
        self.auth_manager = AuthStateManager()
        self.setWindowTitle("Plant Intelligence Assistant - 植物智能助手终端")
        self.resize(1080, 760)
        self.setMinimumSize(860, 640)
        self.stack = QStackedWidget()
        self.stack.setStyleSheet("QStackedWidget { background: #eef8f1; }")
        self.setCentralWidget(self.stack)
        self._build_pages()
        self._route_initial_page()

    def _build_pages(self) -> None:
        self.login_page = LoginPage(self.auth_manager)
        self.wake_page = WakePage(self.auth_manager)
        self.dashboard_page = DashboardPage(self.auth_manager)

        self.login_page.login_success.connect(self._show_wake_page)
        self.wake_page.wake_requested.connect(self._show_dashboard_page)
        self.dashboard_page.logout_requested.connect(self._logout)

        self.stack.addWidget(self.login_page)
        self.stack.addWidget(self.wake_page)
        self.stack.addWidget(self.dashboard_page)

    def _route_initial_page(self) -> None:
        if self.auth_manager.has_logged_in_session():
            self._show_wake_page()
        else:
            self.stack.setCurrentWidget(self.login_page)
            self._set_window_size(1080, 760)

    def _show_wake_page(self) -> None:
        self.dashboard_page._refresh_dashboard()
        self.stack.setCurrentWidget(self.wake_page)
        self._set_window_size(1080, 760)

    def _show_dashboard_page(self) -> None:
        self.dashboard_page._refresh_dashboard()
        self.stack.setCurrentWidget(self.dashboard_page)
        self._set_window_size(1080, 930)

    def _logout(self) -> None:
        reply = QMessageBox.question(
            self,
            "退出登录",
            "确认退出当前账号吗？",
            QMessageBox.Yes | QMessageBox.No,
            QMessageBox.No,
        )
        if reply == QMessageBox.Yes:
            self.auth_manager.logout_user()
            self.stack.setCurrentWidget(self.login_page)
            self._set_window_size(1080, 760)

    def _set_window_size(self, width: int, height: int) -> None:
        self.showNormal()
        self.resize(width, height)
        screen = QDesktopWidget().availableGeometry(self)
        x = screen.x() + (screen.width() - self.width()) // 2
        y = screen.y() + (screen.height() - self.height()) // 2
        self.move(x, y)


def run() -> int:
    app = QApplication.instance() or QApplication([])
    app.setStyle("Fusion")
    window = PlantAssistantMainWindow()
    window.show()
    return app.exec_()
