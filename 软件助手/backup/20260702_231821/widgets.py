import math
from dataclasses import dataclass
from datetime import datetime
from typing import List, Optional

from PyQt5.QtCore import QPointF, QRectF, Qt, QTimer
from PyQt5.QtGui import (
    QColor,
    QFont,
    QLinearGradient,
    QPainter,
    QPainterPath,
    QPen,
)
from PyQt5.QtWidgets import QPushButton, QSizePolicy, QWidget


class EyeToggleButton(QPushButton):
    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        self.setCheckable(True)
        self.setCursor(Qt.PointingHandCursor)
        self.setFixedSize(28, 28)
        self.setStyleSheet(
            "QPushButton { border: none; background: transparent; }"
            "QPushButton:hover { background: rgba(61, 189, 98, 0.08); border-radius: 14px; }"
        )

    def paintEvent(self, event) -> None:
        super().paintEvent(event)
        painter = QPainter(self)
        painter.setRenderHint(QPainter.Antialiasing)
        color = QColor("#688c76")
        pen = QPen(color, 1.7)
        painter.setPen(pen)
        rect = self.rect().adjusted(6, 8, -6, -8)
        path = QPainterPath()
        path.moveTo(rect.left(), rect.center().y())
        path.quadTo(rect.center().x(), rect.top(), rect.right(), rect.center().y())
        path.quadTo(rect.center().x(), rect.bottom(), rect.left(), rect.center().y())
        painter.drawPath(path)
        if self.isChecked():
            painter.setBrush(color)
            painter.drawEllipse(QPointF(rect.center().x(), rect.center().y()), 2.7, 2.7)
        else:
            painter.drawLine(rect.left(), rect.bottom(), rect.right(), rect.top())


class PillButton(QPushButton):
    def __init__(self, text: str, accent: str = "#35be63", danger: bool = False, parent=None) -> None:
        super().__init__(text, parent)
        self._accent = accent
        self._danger = danger
        self.setCursor(Qt.PointingHandCursor)
        self.setMinimumHeight(48)
        self.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Fixed)
        self.setFont(QFont("Microsoft YaHei UI", 10, QFont.Bold))
        self._apply_style()

    def _apply_style(self) -> None:
        if self._danger:
            bg = "#fff3f1"
            border = "#f3c7bf"
            fg = "#ff5b43"
            hover = "#ffe8e4"
        else:
            bg = self._accent
            border = self._accent
            fg = "#ffffff"
            hover = "#2dad58"
        self.setStyleSheet(
            f"""
            QPushButton {{
                background: {bg};
                color: {fg};
                border: 1px solid {border};
                border-radius: 18px;
                padding: 8px 18px;
            }}
            QPushButton:hover {{
                background: {hover};
            }}
            QPushButton:pressed {{
                padding-top: 9px;
            }}
            """
        )


class ToggleActionButton(PushButton := QPushButton):
    def __init__(self, text: str, active_text: Optional[str] = None, parent=None) -> None:
        super().__init__(text, parent)
        self.base_text = text
        self.active_text = active_text or text
        self.setCheckable(True)
        self.setCursor(Qt.PointingHandCursor)
        self.setMinimumHeight(46)
        self.setFont(QFont("Microsoft YaHei UI", 10, QFont.Bold))
        self.toggled.connect(self._sync_style)
        self._sync_style(False)

    def _sync_style(self, checked: bool) -> None:
        self.setText(self.active_text if checked else self.base_text)
        bg = "#dff1e8" if checked else "#f3f8f4"
        border = "#35be63" if checked else "#d9e8de"
        fg = "#2c8450" if checked else "#37684a"
        self.setStyleSheet(
            f"""
            QPushButton {{
                background: {bg};
                color: {fg};
                border: 1px solid {border};
                border-radius: 18px;
                padding: 8px 16px;
            }}
            QPushButton:hover {{
                background: {'#d5eddf' if checked else '#ebf4ee'};
            }}
            """
        )


class PlantAvatarWidget(QWidget):
    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        self.setFixedSize(42, 42)

    def paintEvent(self, event) -> None:
        painter = QPainter(self)
        painter.setRenderHint(QPainter.Antialiasing)
        center = QPointF(self.width() / 2, self.height() / 2)

        painter.setPen(Qt.NoPen)
        painter.setBrush(QColor("#eef7f1"))
        painter.drawEllipse(self.rect().adjusted(1, 1, -1, -1))

        stem_pen = QPen(QColor("#58b947"), 2.0)
        stem_pen.setCapStyle(Qt.RoundCap)
        painter.setPen(stem_pen)
        painter.drawLine(QPointF(center.x(), center.y() + 11), QPointF(center.x(), center.y() - 8))

        painter.setPen(Qt.NoPen)
        painter.setBrush(QColor("#7edc55"))
        left_leaf = QPainterPath()
        left_leaf.moveTo(center.x() - 1, center.y() - 3)
        left_leaf.cubicTo(center.x() - 15, center.y() - 12, center.x() - 15, center.y() + 7, center.x() - 3, center.y() + 3)
        left_leaf.cubicTo(center.x() - 6, center.y() + 1, center.x() - 4, center.y() - 1, center.x() - 1, center.y() - 3)
        painter.drawPath(left_leaf)

        right_leaf = QPainterPath()
        right_leaf.moveTo(center.x() + 1, center.y() - 7)
        right_leaf.cubicTo(center.x() + 15, center.y() - 17, center.x() + 18, center.y() + 1, center.x() + 4, center.y() + 1)
        right_leaf.cubicTo(center.x() + 8, center.y() - 2, center.x() + 5, center.y() - 5, center.x() + 1, center.y() - 7)
        painter.drawPath(right_leaf)


class SoilMoistureBar(QWidget):
    def __init__(self, value: float = 0.0, parent=None) -> None:
        super().__init__(parent)
        self._value = value
        self.setMinimumHeight(26)

    def setValue(self, value: float) -> None:
        self._value = max(0.0, min(100.0, value))
        self.update()

    def paintEvent(self, event) -> None:
        painter = QPainter(self)
        painter.setRenderHint(QPainter.Antialiasing)
        rect = self.rect().adjusted(0, 3, -1, -3)
        painter.setPen(Qt.NoPen)
        painter.setBrush(QColor("#dceae5"))
        painter.drawRoundedRect(rect, 10, 10)
        fill_width = int(rect.width() * self._value / 100.0)
        if fill_width > 0:
            fill_rect = QRectF(rect.left(), rect.top(), fill_width, rect.height())
            gradient = QLinearGradient(fill_rect.topLeft(), fill_rect.topRight())
            gradient.setColorAt(0.0, QColor("#1aa5de"))
            gradient.setColorAt(1.0, QColor("#1aa5de"))
            painter.setBrush(gradient)
            painter.drawRoundedRect(fill_rect, 10, 10)


@dataclass
class RadarObstacle:
    angle: Optional[float] = None
    distance: Optional[float] = None


class RadarWidget(QWidget):
    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        self.sweep_angle = 55.0
        self.obstacle = RadarObstacle()
        self._timer = QTimer(self)
        self._timer.timeout.connect(self._advance_sweep)
        self._timer.start(40)
        self.setMinimumSize(188, 188)

    def set_obstacle(self, angle: Optional[float], distance: Optional[float]) -> None:
        self.obstacle = RadarObstacle(angle=angle, distance=distance)
        self.update()

    def _advance_sweep(self) -> None:
        self.sweep_angle = (self.sweep_angle + 3.0) % 360.0
        self.update()

    def _to_qt_angle(self, angle: float) -> float:
        return 180.0 - angle

    def _point_for_polar(self, center: QPointF, radius: float, angle: float) -> QPointF:
        qt_angle = math.radians(self._to_qt_angle(angle))
        return QPointF(center.x() + radius * math.cos(qt_angle), center.y() - radius * math.sin(qt_angle))

    def paintEvent(self, event) -> None:
        painter = QPainter(self)
        painter.setRenderHint(QPainter.Antialiasing)
        bounds = self.rect().adjusted(8, 8, -8, -8)
        diameter = min(bounds.width(), bounds.height())
        radar_rect = QRectF(bounds.center().x() - diameter / 2, bounds.center().y() - diameter / 2, diameter, diameter)
        center = radar_rect.center()
        radius = radar_rect.width() / 2

        painter.setPen(Qt.NoPen)
        painter.setBrush(QColor("#0c2418"))
        painter.drawEllipse(radar_rect)

        painter.setClipRect(self.rect())
        sector_path = QPainterPath()
        sector_path.moveTo(center)
        sector_path.arcTo(radar_rect, self._to_qt_angle(self.sweep_angle), -52)
        sector_path.closeSubpath()
        gradient = QLinearGradient(center, self._point_for_polar(center, radius, self.sweep_angle))
        gradient.setColorAt(0.0, QColor(60, 255, 130, 35))
        gradient.setColorAt(1.0, QColor(60, 255, 130, 155))
        painter.fillPath(sector_path, gradient)

        pen = QPen(QColor("#1dcf67"), 1)
        pen.setStyle(Qt.DotLine)
        painter.setPen(pen)
        for factor in (0.33, 0.66, 1.0):
            painter.drawEllipse(center, radius * factor, radius * factor)
        painter.drawLine(QPointF(center.x() - radius, center.y()), QPointF(center.x() + radius, center.y()))
        painter.drawLine(QPointF(center.x(), center.y() - radius), QPointF(center.x(), center.y() + radius))

        if self.obstacle.angle is not None and self.obstacle.distance is not None and self.obstacle.distance <= 200:
            clamped_radius = radius * max(0.08, min(1.0, self.obstacle.distance / 200.0))
            point = self._point_for_polar(center, clamped_radius, self.obstacle.angle)
            painter.setPen(Qt.NoPen)
            painter.setBrush(QColor("#ff5944") if self.obstacle.distance < 100 else QColor("#24d56e"))
            painter.drawEllipse(point, 5.5, 5.5)


class MoistureCurveWidget(QWidget):
    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        self.points: List[float] = [42.0, 42.0, 42.0, 42.0, 42.0, 42.0]
        self.time_labels: List[str] = [datetime.now().strftime("%H:%M:%S")] * 3
        self.setMinimumHeight(194)

    def set_curve_data(self, points: List[float], labels: List[str]) -> None:
        if points:
            self.points = points
        if labels:
            self.time_labels = labels[:3]
        self.update()

    def paintEvent(self, event) -> None:
        painter = QPainter(self)
        painter.setRenderHint(QPainter.Antialiasing)
        rect = self.rect().adjusted(22, 16, -24, -42)
        chart_rect = QRectF(rect.left() + 42, rect.top() + 10, rect.width() - 50, rect.height() - 34)
        painter.setPen(QPen(QColor("#dce8e0"), 1))
        painter.drawRect(chart_rect)

        dotted_pen = QPen(QColor("#dce8e0"), 1)
        dotted_pen.setStyle(Qt.DotLine)
        painter.setPen(dotted_pen)
        for factor in (0.25, 0.5, 0.75):
            y = chart_rect.top() + chart_rect.height() * factor
            painter.drawLine(chart_rect.left(), y, chart_rect.right(), y)

        painter.setPen(QColor("#3d8d5b"))
        painter.setFont(QFont("Microsoft YaHei UI", 11))
        painter.drawText(QRectF(rect.left(), chart_rect.top() - 8, 60, 24), "100%")
        painter.drawText(QRectF(rect.left(), chart_rect.bottom() + 6, 90, 22), "土壤湿度")

        if not self.points:
            return

        line_pen = QPen(QColor("#35be63"), 2)
        painter.setPen(line_pen)
        count = len(self.points)
        step = chart_rect.width() / max(1, count - 1)
        polygon = []
        for index, value in enumerate(self.points):
            x = chart_rect.left() + step * index
            y = chart_rect.bottom() - chart_rect.height() * (value / 100.0)
            polygon.append(QPointF(x, y))
        for idx in range(len(polygon) - 1):
            painter.drawLine(polygon[idx], polygon[idx + 1])

        painter.setBrush(QColor("#35be63"))
        painter.setPen(Qt.NoPen)
        for point in polygon:
            painter.drawEllipse(point, 3.2, 3.2)

        fill_path = QPainterPath()
        fill_path.moveTo(polygon[0].x(), chart_rect.bottom())
        for point in polygon:
            fill_path.lineTo(point)
        fill_path.lineTo(polygon[-1].x(), chart_rect.bottom())
        fill_path.closeSubpath()
        painter.fillPath(fill_path, QColor(53, 190, 99, 28))

        painter.setPen(QColor("#385b48"))
        labels = self.time_labels[:]
        while len(labels) < 3:
            labels.append(labels[-1] if labels else "--:--:--")
        painter.drawText(QRectF(chart_rect.left() - 8, chart_rect.bottom() + 26, 80, 20), labels[0])
        painter.drawText(QRectF(chart_rect.center().x() - 35, chart_rect.bottom() + 26, 80, 20), labels[1])
        painter.drawText(QRectF(chart_rect.right() - 66, chart_rect.bottom() + 26, 80, 20), labels[2])
