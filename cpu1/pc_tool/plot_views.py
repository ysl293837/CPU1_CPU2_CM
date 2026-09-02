from __future__ import annotations

from typing import Dict, Optional

import pyqtgraph as pg
from PyQt5 import QtCore, QtGui, QtWidgets


CHANNEL_LABELS = {
    "enc_count": "Encoder Count",
    "current_speed": "Current Speed (r/s)",
    "target_speed": "Target Speed (r/s)",
    "pid_output": "PID Output",
    "i_term": "I Term",
    "run": "Run",
    "estop": "E-Stop",
    "kp": "Kp",
    "ki": "Ki",
    "kd": "Kd",
}

DEFAULT_X_WINDOW_SECONDS = 10.0
DEFAULT_Y_PADDING_RATIO = 0.08
ZERO_LOCK_RATIO = 0.02


class PlotPanelWidget(QtWidgets.QWidget):
    def __init__(
        self,
        panel_title: str,
        channels: Dict[str, object],
        auto_select_new_channels: bool,
        parent: Optional[QtWidgets.QWidget] = None,
    ) -> None:
        super().__init__(parent)
        self.panel_title = panel_title
        self.channels = channels
        self.auto_select_new_channels = auto_select_new_channels
        self.channel_rows: Dict[str, int] = {}
        self.curves: Dict[str, object] = {}
        self.plot_paused = False
        self._pending_plot_update = False
        self._plot_update_scheduled = False
        self._last_plot_xrange = DEFAULT_X_WINDOW_SECONDS
        self._auto_follow_latest = True
        self.last_line_count = 0
        self.last_rate = 0.0
        self._build_ui()

    def _build_ui(self) -> None:
        root_layout = QtWidgets.QVBoxLayout(self)
        root_layout.setContentsMargins(0, 0, 0, 0)
        root_layout.setSpacing(8)

        controls_row = QtWidgets.QHBoxLayout()
        root_layout.addLayout(controls_row)

        title_label = QtWidgets.QLabel(self.panel_title)
        title_font = title_label.font()
        title_font.setPointSize(title_font.pointSize() + 1)
        title_font.setBold(True)
        title_label.setFont(title_font)
        controls_row.addWidget(title_label)

        controls_row.addSpacing(10)
        controls_row.addWidget(QtWidgets.QLabel("Window"))

        self.x_window_spin = QtWidgets.QDoubleSpinBox()
        self.x_window_spin.setRange(1.0, 120.0)
        self.x_window_spin.setDecimals(1)
        self.x_window_spin.setSingleStep(1.0)
        self.x_window_spin.setValue(DEFAULT_X_WINDOW_SECONDS)
        self.x_window_spin.setSuffix(" s")
        self.x_window_spin.valueChanged.connect(self._set_x_window)
        controls_row.addWidget(self.x_window_spin)

        self.y_auto_check = QtWidgets.QCheckBox("Auto Y")
        self.y_auto_check.setChecked(True)
        self.y_auto_check.toggled.connect(self._on_y_auto_toggled)
        controls_row.addWidget(self.y_auto_check)

        self.reset_y_button = QtWidgets.QPushButton("Reset Y")
        self.reset_y_button.clicked.connect(self._reset_y_view)
        controls_row.addWidget(self.reset_y_button)

        self.follow_latest_check = QtWidgets.QCheckBox("Follow Latest")
        self.follow_latest_check.setChecked(True)
        self.follow_latest_check.toggled.connect(self._set_auto_follow)
        controls_row.addWidget(self.follow_latest_check)

        self.pause_button = QtWidgets.QPushButton("Pause Plot")
        self.pause_button.clicked.connect(self._toggle_pause)
        controls_row.addWidget(self.pause_button)

        self.draw_all_button = QtWidgets.QPushButton("Draw All")
        self.draw_all_button.clicked.connect(lambda: self._set_all_channel_checks(True))
        controls_row.addWidget(self.draw_all_button)

        self.hide_all_button = QtWidgets.QPushButton("Hide All")
        self.hide_all_button.clicked.connect(lambda: self._set_all_channel_checks(False))
        controls_row.addWidget(self.hide_all_button)
        controls_row.addStretch(1)

        splitter = QtWidgets.QSplitter(QtCore.Qt.Horizontal)
        splitter.setChildrenCollapsible(False)
        root_layout.addWidget(splitter, 1)

        self.channel_table = QtWidgets.QTableWidget(0, 2)
        self.channel_table.setHorizontalHeaderLabels(["Channel", "Latest"])
        self.channel_table.horizontalHeader().setStretchLastSection(True)
        self.channel_table.horizontalHeader().setSectionResizeMode(
            0, QtWidgets.QHeaderView.Stretch
        )
        self.channel_table.verticalHeader().setVisible(False)
        self.channel_table.setAlternatingRowColors(True)
        self.channel_table.setSelectionMode(QtWidgets.QAbstractItemView.NoSelection)
        self.channel_table.setMinimumWidth(280)
        self.channel_table.setMaximumWidth(390)
        self.channel_table.itemChanged.connect(self._sync_curve_visibility)
        splitter.addWidget(self.channel_table)

        self.plot_widget = pg.PlotWidget()
        self.plot_widget.setBackground("#fbfcfe")
        self.plot_widget.showGrid(x=True, y=True, alpha=0.2)
        self.plot_widget.setLabel("left", "Value")
        self.plot_widget.setLabel("bottom", "Time", units="s")
        self.plot_widget.addLegend(offset=(10, 10))
        self.plot_widget.getPlotItem().layout.setContentsMargins(8, 4, 12, 8)
        self.plot_widget.getAxis("left").setWidth(72)
        self.plot_widget.getAxis("bottom").setHeight(42)
        self.plot_widget.setMouseEnabled(x=True, y=True)
        self.plot_widget.getViewBox().setLimits(xMin=0)
        self.plot_widget.viewport().installEventFilter(self)
        splitter.addWidget(self.plot_widget)
        splitter.setStretchFactor(1, 1)
        splitter.setSizes([330, 1100])
        self._refresh_title()

    def ensure_channel(self, name: str, channel: object) -> None:
        if name in self.channel_rows:
            self.set_latest_value(name, getattr(channel, "last_value", None))
            return

        checked = self.auto_select_new_channels
        display_name = CHANNEL_LABELS.get(name, name)
        curve = self.plot_widget.plot(
            [],
            [],
            pen=pg.mkPen(color=getattr(channel, "color", "#006d77"), width=1.7),
            name=display_name,
        )
        curve.setClipToView(True)
        curve.setDownsampling(auto=True, method="peak")
        self.curves[name] = curve

        row = self.channel_table.rowCount()
        self.channel_table.insertRow(row)
        self.channel_rows[name] = row

        name_item = QtWidgets.QTableWidgetItem(display_name)
        name_item.setFlags(name_item.flags() | QtCore.Qt.ItemIsUserCheckable)
        name_item.setCheckState(QtCore.Qt.Checked if checked else QtCore.Qt.Unchecked)
        name_item.setForeground(pg.mkColor(getattr(channel, "color", "#006d77")))
        name_item.setData(QtCore.Qt.UserRole, name)

        latest_item = QtWidgets.QTableWidgetItem(
            "-" if getattr(channel, "last_value", None) is None else self._format_value(channel.last_value)
        )
        latest_item.setTextAlignment(QtCore.Qt.AlignRight | QtCore.Qt.AlignVCenter)

        self.channel_table.blockSignals(True)
        self.channel_table.setItem(row, 0, name_item)
        self.channel_table.setItem(row, 1, latest_item)
        self.channel_table.blockSignals(False)
        self._set_curve_visible(name, checked)
        self._refresh_title()

    def set_latest_value(self, name: str, value: Optional[float]) -> None:
        row = self.channel_rows.get(name)
        if row is None:
            return
        item = self.channel_table.item(row, 1)
        if item is not None:
            item.setText("-" if value is None else self._format_value(value))

    def update_status(self, line_count: int, rate: float) -> None:
        self.last_line_count = line_count
        self.last_rate = rate
        self._refresh_title()

    def clear_data(self) -> None:
        for curve in self.curves.values():
            curve.setData([], [])
        for row in range(self.channel_table.rowCount()):
            item = self.channel_table.item(row, 1)
            if item is not None:
                item.setText("-")
        self.mark_dirty()

    def mark_dirty(self) -> None:
        self._pending_plot_update = True
        self._schedule_plot_update()

    def _toggle_pause(self) -> None:
        self.plot_paused = not self.plot_paused
        self.pause_button.setText("Resume Plot" if self.plot_paused else "Pause Plot")
        if not self.plot_paused:
            self.mark_dirty()

    def _set_x_window(self, _value: float = 0.0) -> None:
        self._last_plot_xrange = self.x_window_spin.value()
        if self._auto_follow_latest:
            self._set_auto_follow(True)
        self.mark_dirty()

    def _on_y_auto_toggled(self, checked: bool) -> None:
        if checked:
            self.mark_dirty()

    def _reset_y_view(self) -> None:
        self.y_auto_check.setChecked(True)
        self.mark_dirty()

    def _set_auto_follow(self, checked: bool) -> None:
        self._auto_follow_latest = checked
        if self.follow_latest_check.isChecked() != checked:
            self.follow_latest_check.blockSignals(True)
            self.follow_latest_check.setChecked(checked)
            self.follow_latest_check.blockSignals(False)
        if checked:
            self.mark_dirty()

    def eventFilter(self, obj: QtCore.QObject, event: QtCore.QEvent) -> bool:
        if obj is self.plot_widget.viewport():
            event_type = event.type()
            if event_type == QtCore.QEvent.MouseButtonDblClick:
                self.y_auto_check.setChecked(True)
                self._set_auto_follow(True)
                return True
            if event_type in (QtCore.QEvent.MouseButtonPress, QtCore.QEvent.Wheel):
                self.y_auto_check.setChecked(False)
                self._set_auto_follow(False)
        return super().eventFilter(obj, event)

    def _set_all_channel_checks(self, checked: bool) -> None:
        state = QtCore.Qt.Checked if checked else QtCore.Qt.Unchecked
        self.channel_table.blockSignals(True)
        for row in range(self.channel_table.rowCount()):
            item = self.channel_table.item(row, 0)
            if item is not None:
                item.setCheckState(state)
        self.channel_table.blockSignals(False)
        self.mark_dirty()
        self._refresh_title()

    def _set_curve_visible(self, name: str, visible: bool) -> None:
        curve = self.curves.get(name)
        if curve is None:
            return
        curve.setVisible(visible)
        legend = self.plot_widget.plotItem.legend
        if legend is None:
            return
        for sample, label in legend.items:
            if getattr(sample, "item", None) is curve:
                sample.setVisible(visible)
                label.setVisible(visible)
                break

    def _sync_curve_visibility(self, item: QtWidgets.QTableWidgetItem) -> None:
        if item.column() != 0:
            return
        self.mark_dirty()
        self._refresh_title()

    def _schedule_plot_update(self) -> None:
        if self._plot_update_scheduled:
            return
        self._plot_update_scheduled = True
        QtCore.QTimer.singleShot(0, self._update_plot)

    def _update_plot(self) -> None:
        self._plot_update_scheduled = False
        if self.plot_paused or not self._pending_plot_update:
            return
        self._pending_plot_update = False

        visible_values = []
        newest_t = 0.0
        for name, curve in self.curves.items():
            row = self.channel_rows[name]
            item = self.channel_table.item(row, 0)
            channel = self.channels.get(name)
            visible = item is not None and item.checkState() == QtCore.Qt.Checked
            self._set_curve_visible(name, visible)

            if not visible or channel is None or not getattr(channel, "times", None):
                curve.setData([], [])
                continue

            xs = list(channel.times)
            ys = list(channel.values)
            curve.setData(xs, ys)
            newest_t = max(newest_t, xs[-1])

        view_range = self.plot_widget.getViewBox().viewRange()[0]
        xmin = float(view_range[0])
        xmax = float(view_range[1])
        if self._auto_follow_latest:
            xmin = max(0.0, newest_t - self._last_plot_xrange)
            xmax = max(self._last_plot_xrange, newest_t)
            self.plot_widget.setXRange(xmin, xmax, padding=0.0)

        if self.y_auto_check.isChecked():
            for name, channel in self.channels.items():
                row = self.channel_rows.get(name)
                if row is None:
                    continue
                item = self.channel_table.item(row, 0)
                visible = item is not None and item.checkState() == QtCore.Qt.Checked
                if not visible or not getattr(channel, "times", None):
                    continue
                for x, y in zip(channel.times, channel.values):
                    if x >= xmin:
                        visible_values.append(y)
            if visible_values:
                ymin, ymax = self._calculate_auto_y_range(visible_values)
                self.plot_widget.setYRange(ymin, ymax, padding=0.0)

        self._refresh_title()

    def _calculate_auto_y_range(self, visible_values) -> tuple[float, float]:
        ymin = min(visible_values)
        ymax = max(visible_values)

        if ymin >= 0.0:
            ymin = 0.0
        elif ymax <= 0.0:
            ymax = 0.0
        else:
            positive_peak = abs(ymax)
            negative_peak = abs(ymin)
            if negative_peak <= max(positive_peak * ZERO_LOCK_RATIO, 1e-3):
                ymin = 0.0
            elif positive_peak <= max(negative_peak * ZERO_LOCK_RATIO, 1e-3):
                ymax = 0.0

        if ymin == ymax:
            if ymax == 0.0:
                return (-1.0, 1.0)
            pad = max(abs(ymin) * 0.05, 1e-3)
            return (ymin - pad, ymax + pad)

        pad = (ymax - ymin) * DEFAULT_Y_PADDING_RATIO
        if ymin == 0.0 and ymax > 0.0:
            return (0.0, ymax + pad)
        if ymax == 0.0 and ymin < 0.0:
            return (ymin - pad, 0.0)
        return (ymin - pad, ymax + pad)

    def _selected_channel_count(self) -> int:
        selected = 0
        for row in range(self.channel_table.rowCount()):
            item = self.channel_table.item(row, 0)
            if item is not None and item.checkState() == QtCore.Qt.Checked:
                selected += 1
        return selected

    def _refresh_title(self) -> None:
        self.plot_widget.setTitle(
            "{} | Lines: {} | Rate: {:.1f} lines/s | Curves: {}".format(
                self.panel_title,
                self.last_line_count,
                self.last_rate,
                self._selected_channel_count(),
            )
        )

    def _format_value(self, value: float) -> str:
        if abs(value - round(value)) < 1e-6:
            return str(int(round(value)))
        return "{:.4f}".format(value)


class PlotPanelWindow(QtWidgets.QMainWindow):
    closed = QtCore.pyqtSignal(object)

    def __init__(
        self,
        panel_title: str,
        channels: Dict[str, object],
        auto_select_new_channels: bool,
    ) -> None:
        super().__init__()
        self.panel = PlotPanelWidget(
            panel_title=panel_title,
            channels=channels,
            auto_select_new_channels=auto_select_new_channels,
        )
        self.setCentralWidget(self.panel)
        self.setAttribute(QtCore.Qt.WA_DeleteOnClose, True)
        self.setWindowTitle(panel_title)
        self.resize(1180, 680)

    def closeEvent(self, event: QtGui.QCloseEvent) -> None:
        self.closed.emit(self)
        super().closeEvent(event)
