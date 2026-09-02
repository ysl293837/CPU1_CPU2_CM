from __future__ import annotations

import csv
import os
import queue
import re
import sys
import time
from collections import deque
from dataclasses import dataclass, field
from pathlib import Path
from typing import Deque, Dict, List, Optional, Tuple

import pyqtgraph as pg
import serial
from PyQt5 import QtCore, QtGui, QtWidgets
from serial.tools import list_ports

from plot_views import PlotPanelWidget, PlotPanelWindow


APP_STYLESHEET = """
QMainWindow, QWidget {
    background: #f3f6fb;
    color: #102a43;
}
QLabel {
    color: #243b53;
}
QGroupBox {
    border: 1px solid #d9e2ec;
    border-radius: 8px;
    margin-top: 10px;
    padding-top: 10px;
    font-weight: 600;
    background: #ffffff;
}
QGroupBox::title {
    subcontrol-origin: margin;
    left: 10px;
    padding: 0 4px;
}
QLineEdit, QSpinBox, QDoubleSpinBox, QComboBox, QPlainTextEdit {
    background: #ffffff;
    border: 1px solid #cbd2d9;
    border-radius: 6px;
    padding: 6px 8px;
}
QPushButton {
    background: #0f766e;
    color: #ffffff;
    border: none;
    border-radius: 6px;
    padding: 7px 12px;
    font-weight: 600;
}
QPushButton:hover {
    background: #115e59;
}
QPushButton:pressed {
    background: #134e4a;
}
QCheckBox {
    spacing: 6px;
}
QHeaderView::section {
    background: #e9eef5;
    border: none;
    padding: 6px;
    color: #243b53;
    font-weight: 600;
}
QSplitter::handle {
    background: #d9e2ec;
}
"""


NUMBER_PATTERN = r"[-+]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][-+]?\d+)?"
KV_PATTERN = re.compile(
    r"([A-Za-z_][A-Za-z0-9_.\[\]]*)\s*[:=]\s*({})".format(NUMBER_PATTERN)
)
COLORS = [
    "#006d77",
    "#e76f51",
    "#264653",
    "#f4a261",
    "#2a9d8f",
    "#8d99ae",
    "#bc4749",
    "#3a86ff",
    "#ff006e",
    "#6a994e",
]
PLOT_CHANNEL_KEYS = {
    "enc_count",
    "current_speed",
    "target_speed",
    "pid_output",
    "i_term",
}
DEFAULT_CAPTURE_SAMPLE_PERIOD_SECONDS = 1.0 / 20000.0


def parse_key_value_line(line: str) -> Dict[str, float]:
    parsed: Dict[str, float] = {}
    normalized = line.strip().strip("\x00")
    for key, value_text in KV_PATTERN.findall(line):
        try:
            parsed[key.strip()] = float(value_text)
        except ValueError:
            pass
    if not parsed and normalized:
        parts = [part.strip() for part in re.split(r"[,;\s]+", normalized) if part.strip()]
        numeric_values: List[float] = []
        for part in parts:
            try:
                numeric_values.append(float(part))
            except ValueError:
                numeric_values = []
                break
        if len(numeric_values) >= 3:
            for idx, value in enumerate(numeric_values):
                parsed["ch{}".format(idx + 1)] = value
    return parsed


@dataclass
class ChannelSeries:
    color: str
    times: Deque[float] = field(default_factory=lambda: deque(maxlen=4000))
    values: Deque[float] = field(default_factory=lambda: deque(maxlen=4000))
    last_value: Optional[float] = None

    def set_capacity(self, capacity: int) -> None:
        self.times = deque(self.times, maxlen=capacity)
        self.values = deque(self.values, maxlen=capacity)


class SerialWorker(QtCore.QObject):
    lines_received = QtCore.pyqtSignal(list)
    raw_received = QtCore.pyqtSignal(int, str)
    status_changed = QtCore.pyqtSignal(str)
    finished = QtCore.pyqtSignal()

    def __init__(self, port: str, baudrate: int, parent: Optional[QtCore.QObject] = None) -> None:
        super().__init__(parent)
        self.port = port
        self.baudrate = baudrate
        self._serial: Optional[serial.Serial] = None
        self._running = True
        self._tx_queue: "queue.Queue[str]" = queue.Queue()
        self._rx_buffer = bytearray()

    @QtCore.pyqtSlot()
    def run(self) -> None:
        try:
            self._serial = serial.Serial()
            self._serial.port = self.port
            self._serial.baudrate = self.baudrate
            self._serial.bytesize = serial.EIGHTBITS
            self._serial.parity = serial.PARITY_NONE
            self._serial.stopbits = serial.STOPBITS_ONE
            self._serial.timeout = 0
            self._serial.write_timeout = 0.2
            if hasattr(self._serial, "exclusive") and os.name != "nt":
                self._serial.exclusive = False
            self._serial.rts = True
            self._serial.dtr = True
            self._serial.open()
            try:
                self._serial.setRTS(True)
                self._serial.setDTR(True)
            except Exception:
                pass
            self._serial.reset_output_buffer()
            self.status_changed.emit("Connected {} @ {}".format(self.port, self.baudrate))
        except Exception as exc:
            self.status_changed.emit("Serial open failed: {}".format(exc))
            self.finished.emit()
            return

        try:
            last_idle = time.perf_counter()
            while self._running:
                self._drain_tx_queue()
                batch = self._read_lines_batch(max_lines=1000)
                if batch:
                    self.lines_received.emit(batch)
                    last_idle = time.perf_counter()
                else:
                    now = time.perf_counter()
                    if now - last_idle > 0.002:
                        QtCore.QThread.msleep(1)
        finally:
            self._drain_tx_queue()
            try:
                if self._serial is not None and self._serial.is_open:
                    self._serial.close()
            except Exception:
                pass
            self.status_changed.emit("Serial disconnected")
            self.finished.emit()

    def _read_lines_batch(self, max_lines: int) -> List[Tuple[str, Dict[str, float]]]:
        if self._serial is None or not self._serial.is_open:
            return []

        out: List[Tuple[str, Dict[str, float]]] = []
        try:
            waiting = self._serial.in_waiting
            if waiting:
                chunk = self._serial.read(waiting)
                self._rx_buffer.extend(chunk)
                self._emit_raw_received(chunk)
            else:
                more = self._serial.read(4096)
                if more:
                    self._rx_buffer.extend(more)
                    self._emit_raw_received(more)
        except Exception as exc:
            self.status_changed.emit("Serial read failed: {}".format(exc))
            self._running = False
            return []

        while len(out) < max_lines:
            pos = self._rx_buffer.find(b"\n")
            if pos < 0:
                break
            raw = self._rx_buffer[: pos + 1]
            del self._rx_buffer[: pos + 1]
            line = raw.decode("utf-8", errors="ignore").strip()
            if line:
                out.append((line, parse_key_value_line(line)))

        if len(self._rx_buffer) > 1024 * 1024:
            self._rx_buffer = self._rx_buffer[-65536:]

        return out

    def _emit_raw_received(self, chunk: bytes) -> None:
        if not chunk:
            return
        preview = chunk[:80].decode("utf-8", errors="replace").replace("\r", "\\r").replace("\n", "\\n")
        self.raw_received.emit(len(chunk), preview)

    def _drain_tx_queue(self) -> None:
        if self._serial is None or not self._serial.is_open:
            return

        while True:
            try:
                text = self._tx_queue.get_nowait()
            except queue.Empty:
                break
            if not text:
                continue
            payload = text.encode("utf-8", errors="ignore")
            if not text.endswith("\n"):
                payload += b"\r\n"
            try:
                self._serial.write(payload)
            except Exception as exc:
                self.status_changed.emit("Serial send failed: {}".format(exc))
                self._running = False
                break

    def stop(self) -> None:
        self._running = False

    def enqueue_write(self, text: str) -> None:
        self._tx_queue.put(text)


class MotorTunerWindow(QtWidgets.QMainWindow):
    def __init__(self) -> None:
        super().__init__()
        self.setWindowTitle("C2000 PMSM Serial Tuner")
        self.resize(1500, 980)

        pg.setConfigOptions(antialias=False)

        self.worker: Optional[SerialWorker] = None
        self.thread: Optional[QtCore.QThread] = None
        self.channels: Dict[str, ChannelSeries] = {}
        self.records: List[Dict[str, float]] = []
        self.start_time = time.perf_counter()
        self.line_count = 0
        self.rx_byte_count = 0
        self.rate_window_seconds = 0.5
        self.recent_line_times: Deque[float] = deque()
        self._color_index = 0
        self._plot_view_counter = 1
        self._connect_line_count = 0
        self._connect_byte_count = 0
        self._last_raw_preview_log = 0.0
        self._last_line_log_time = 0.0
        self._last_capture_index: Optional[int] = None
        self._capture_missing_count = 0
        self._capture_received_count = 0
        self._capture_sample_period_seconds = DEFAULT_CAPTURE_SAMPLE_PERIOD_SECONDS
        self._capture_base_time = 0.0
        self.plot_panels: List[PlotPanelWidget] = []
        self.plot_windows: List[PlotPanelWindow] = []

        self._build_ui()
        self._register_panel(self.main_plot_panel)
        self._refresh_ports()
        self._update_plot_view_count()

    def _build_ui(self) -> None:
        central = QtWidgets.QWidget()
        self.setCentralWidget(central)

        root_layout = QtWidgets.QVBoxLayout(central)
        root_layout.setContentsMargins(12, 12, 12, 12)
        root_layout.setSpacing(10)

        controls_grid = QtWidgets.QGridLayout()
        controls_grid.setHorizontalSpacing(10)
        controls_grid.setVerticalSpacing(8)
        root_layout.addLayout(controls_grid)

        self.port_combo = QtWidgets.QComboBox()
        self.port_combo.setMinimumWidth(170)
        controls_grid.addWidget(QtWidgets.QLabel("Port"), 0, 0)
        controls_grid.addWidget(self.port_combo, 0, 1)

        self.refresh_button = QtWidgets.QPushButton("Refresh")
        self.refresh_button.clicked.connect(self._refresh_ports)
        controls_grid.addWidget(self.refresh_button, 0, 2)

        self.baud_combo = QtWidgets.QComboBox()
        self.baud_combo.addItems(["115200", "230400", "460800", "921600"])
        self.baud_combo.setCurrentText("115200")
        controls_grid.addWidget(QtWidgets.QLabel("Baud"), 0, 3)
        controls_grid.addWidget(self.baud_combo, 0, 4)

        self.point_spin = QtWidgets.QSpinBox()
        self.point_spin.setRange(200, 100000)
        self.point_spin.setSingleStep(200)
        self.point_spin.setValue(8192)
        self.point_spin.valueChanged.connect(self._apply_capacity)
        controls_grid.addWidget(QtWidgets.QLabel("Points"), 0, 5)
        controls_grid.addWidget(self.point_spin, 0, 6)

        self.connect_button = QtWidgets.QPushButton("Connect")
        self.connect_button.clicked.connect(self._toggle_connection)
        controls_grid.addWidget(self.connect_button, 1, 0)

        self.new_plot_button = QtWidgets.QPushButton("New Plot Window")
        self.new_plot_button.clicked.connect(self._create_additional_plot_window)
        controls_grid.addWidget(self.new_plot_button, 1, 1)

        self.clear_button = QtWidgets.QPushButton("Clear Data")
        self.clear_button.clicked.connect(self._clear_data)
        controls_grid.addWidget(self.clear_button, 1, 2)

        self.export_button = QtWidgets.QPushButton("Export CSV")
        self.export_button.clicked.connect(self._export_csv)
        controls_grid.addWidget(self.export_button, 1, 3)

        self.plot_view_count_label = QtWidgets.QLabel("Plot Views: 0")
        controls_grid.addWidget(self.plot_view_count_label, 1, 4, 1, 2)

        self.rx_byte_label = QtWidgets.QLabel("RX Bytes: 0")
        controls_grid.addWidget(self.rx_byte_label, 1, 6)

        self.status_label = QtWidgets.QLabel("Disconnected")
        self.status_label.setSizePolicy(
            QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Preferred
        )
        controls_grid.addWidget(self.status_label, 1, 7, 1, 2)
        controls_grid.setColumnStretch(8, 1)

        splitter = QtWidgets.QSplitter(QtCore.Qt.Vertical)
        root_layout.addWidget(splitter, 1)

        self.main_plot_panel = PlotPanelWidget(
            panel_title="Plot View 1",
            channels=self.channels,
            auto_select_new_channels=True,
        )
        splitter.addWidget(self.main_plot_panel)

        bottom_panel = QtWidgets.QWidget()
        bottom_layout = QtWidgets.QVBoxLayout(bottom_panel)
        bottom_layout.setContentsMargins(0, 0, 0, 0)
        bottom_layout.setSpacing(8)
        splitter.addWidget(bottom_panel)

        telemetry_group = QtWidgets.QGroupBox("Telemetry")
        telemetry_layout = QtWidgets.QGridLayout(telemetry_group)
        telemetry_layout.setHorizontalSpacing(12)
        telemetry_layout.setVerticalSpacing(8)
        self.telemetry_value_labels: Dict[str, QtWidgets.QLabel] = {}
        telemetry_keys = [
            ("current_speed", "Current Speed"),
            ("target_speed", "Target Speed"),
            ("enc_count", "Encoder Count"),
            ("pid_output", "PID Output"),
            ("i_term", "I Term"),
        ]
        for idx, (key, label_text) in enumerate(telemetry_keys):
            card = QtWidgets.QFrame()
            card.setStyleSheet(
                "QFrame { background: #f8fbff; border: 1px solid #d9e2ec; border-radius: 6px; }"
            )
            card_layout = QtWidgets.QVBoxLayout(card)
            card_layout.setContentsMargins(10, 8, 10, 8)
            card_layout.setSpacing(4)

            name_label = QtWidgets.QLabel(label_text)
            name_label.setStyleSheet("color: #486581;")
            value_label = QtWidgets.QLabel("--")
            value_font = value_label.font()
            value_font.setPointSize(value_font.pointSize() + 4)
            value_font.setBold(True)
            value_label.setFont(value_font)
            value_label.setStyleSheet("color: #102a43;")

            self.telemetry_value_labels[key] = value_label
            card_layout.addWidget(name_label)
            card_layout.addWidget(value_label)
            telemetry_layout.addWidget(card, 0, idx)

        bottom_layout.addWidget(telemetry_group)

        commands_group = QtWidgets.QGroupBox("Command Sender")
        commands_layout = QtWidgets.QVBoxLayout(commands_group)
        commands_layout.setContentsMargins(8, 8, 8, 8)
        commands_layout.setSpacing(6)

        self.command_inputs: List[QtWidgets.QLineEdit] = []
        command_presets = [
            "status",
            "stream=1",
            "period=10",
            "start",
            "stop",
            "estop",
            "clear_estop",
            "target=20",
            "target=0",
            "pid=50,5,0",
            "reset_pid",
        ]

        commands_grid = QtWidgets.QGridLayout()
        commands_grid.setHorizontalSpacing(12)
        commands_grid.setVerticalSpacing(6)
        for idx, default_text in enumerate(command_presets):
            row_layout = QtWidgets.QHBoxLayout()
            label = QtWidgets.QLabel("Cmd {}".format(idx + 1))
            label.setMinimumWidth(52)
            row_layout.addWidget(label)

            line_edit = QtWidgets.QLineEdit()
            line_edit.setText(default_text)
            line_edit.returnPressed.connect(self._make_send_current_handler(idx))
            row_layout.addWidget(line_edit, 1)

            button = QtWidgets.QPushButton("Send")
            button.clicked.connect(self._make_send_button_handler(idx))
            row_layout.addWidget(button)

            self.command_inputs.append(line_edit)
            commands_grid.addLayout(row_layout, idx % 2, idx // 2)

        commands_grid.setColumnStretch(0, 1)
        commands_grid.setColumnStretch(1, 1)
        commands_grid.setColumnStretch(2, 1)
        commands_grid.setColumnStretch(3, 1)
        commands_layout.addLayout(commands_grid)

        quick_row = QtWidgets.QHBoxLayout()
        self.send_interval_spin = QtWidgets.QSpinBox()
        self.send_interval_spin.setRange(0, 2000)
        self.send_interval_spin.setValue(20)
        self.send_interval_spin.setSuffix(" ms")
        quick_row.addWidget(QtWidgets.QLabel("Batch Gap"))
        quick_row.addWidget(self.send_interval_spin)

        self.send_all_button = QtWidgets.QPushButton("Send All Non-Empty")
        self.send_all_button.clicked.connect(self._send_all_commands)
        quick_row.addWidget(self.send_all_button)
        quick_row.addStretch(1)
        commands_layout.addLayout(quick_row)

        bottom_layout.addWidget(commands_group)

        self.raw_log = QtWidgets.QPlainTextEdit()
        self.raw_log.setReadOnly(True)
        self.raw_log.setMaximumBlockCount(3000)
        self.raw_log.setStyleSheet(
            "QPlainTextEdit {"
            " background: #0f172a;"
            " color: #dbeafe;"
            " border-radius: 6px;"
            " padding: 8px;"
            " font-family: Consolas, 'Courier New', monospace;"
            "}"
        )
        bottom_layout.addWidget(self.raw_log, 1)

        splitter.setStretchFactor(0, 3)
        splitter.setStretchFactor(1, 2)

    def _register_panel(self, panel: PlotPanelWidget) -> None:
        if panel in self.plot_panels:
            return
        self.plot_panels.append(panel)
        for name, channel in self.channels.items():
            panel.ensure_channel(name, channel)
        panel.update_status(self.line_count, self._current_receive_rate())
        panel.mark_dirty()

    def _update_plot_view_count(self) -> None:
        self.plot_view_count_label.setText("Plot Views: {}".format(len(self.plot_panels)))

    def _create_additional_plot_window(self, _checked: bool = False) -> None:
        self._plot_view_counter += 1
        window = PlotPanelWindow(
            panel_title="Plot View {}".format(self._plot_view_counter),
            channels=self.channels,
            auto_select_new_channels=False,
        )
        window.closed.connect(self._handle_plot_window_closed)
        self.plot_windows.append(window)
        self._register_panel(window.panel)
        self._update_plot_view_count()
        window.show()
        window.raise_()
        window.activateWindow()

    @QtCore.pyqtSlot(object)
    def _handle_plot_window_closed(self, closed_window: object) -> None:
        if not isinstance(closed_window, PlotPanelWindow):
            return
        if closed_window.panel in self.plot_panels:
            self.plot_panels.remove(closed_window.panel)
        if closed_window in self.plot_windows:
            self.plot_windows.remove(closed_window)
        self._update_plot_view_count()

    def _make_send_current_handler(self, index: int):
        return lambda: self._send_command_from_index(index)

    def _make_send_button_handler(self, index: int):
        return lambda _checked=False: self._send_command_from_index(index)

    def _refresh_ports(self) -> None:
        current = self.port_combo.currentText()
        ports = [port.device for port in list_ports.comports()]
        self.port_combo.blockSignals(True)
        self.port_combo.clear()
        self.port_combo.addItems(ports)
        self.port_combo.blockSignals(False)
        if current in ports:
            self.port_combo.setCurrentText(current)
        self.status_label.setText("Ports refreshed")

    def _toggle_connection(self) -> None:
        if self.worker is not None:
            self._disconnect_serial()
        else:
            self._connect_serial()

    def _connect_serial(self) -> None:
        port = self.port_combo.currentText().strip()
        if not port:
            QtWidgets.QMessageBox.warning(self, "No Port", "No serial port is available.")
            return

        try:
            baudrate = int(self.baud_combo.currentText())
        except ValueError:
            QtWidgets.QMessageBox.warning(self, "Bad Baud", "Baudrate is invalid.")
            return

        self.thread = QtCore.QThread(self)
        self.worker = SerialWorker(port, baudrate)
        self.worker.moveToThread(self.thread)
        self.thread.started.connect(self.worker.run)
        self.worker.lines_received.connect(self._handle_lines)
        self.worker.raw_received.connect(self._handle_raw_bytes)
        self.worker.status_changed.connect(self._set_status)
        self.worker.finished.connect(self._serial_finished)
        self.thread.start()

        self._connect_line_count = self.line_count
        self._connect_byte_count = self.rx_byte_count
        self._last_raw_preview_log = 0.0
        self._last_line_log_time = 0.0
        self.connect_button.setText("Disconnect")
        self.status_label.setText("Connecting {} ...".format(port))
        self._append_log_line("### Opening {} @ {}".format(port, baudrate))
        QtCore.QTimer.singleShot(250, self._queue_startup_commands)
        QtCore.QTimer.singleShot(2200, self._check_no_data_after_connect)

    def _queue_startup_commands(self) -> None:
        if self.worker is None:
            return
        for command in ("stream=1", "period=10", "status"):
            self.worker.enqueue_write(command)
            self._append_log_line(">>> {}".format(command))

    def _check_no_data_after_connect(self) -> None:
        if self.worker is None:
            return
        new_lines = self.line_count - self._connect_line_count
        new_bytes = self.rx_byte_count - self._connect_byte_count
        if new_bytes == 0:
            self.status_label.setText("Connected, but no serial bytes received")
            self._append_log_line(
                "### No RX data. Rebuild/download firmware, press Reset while connected, then check SCIA TX -> USB-RX, SCIA RX <- USB-TX, common GND, and 115200 8N1."
            )
        elif new_lines == 0:
            self.status_label.setText("RX bytes received, but no complete text lines")
            self._append_log_line(
                "### RX bytes are arriving, but no newline-delimited key:value text was parsed. Check baudrate and firmware line endings."
            )

    def _disconnect_serial(self) -> None:
        if self.worker is not None:
            self.worker.stop()
        if self.thread is not None:
            self.thread.quit()
            self.thread.wait(1500)

    def _serial_finished(self) -> None:
        if self.worker is not None:
            self.worker.deleteLater()
            self.worker = None
        if self.thread is not None:
            self.thread.quit()
            self.thread.wait(500)
            self.thread.deleteLater()
            self.thread = None
        self.connect_button.setText("Connect")

    def _apply_capacity(self, _value: int = 0) -> None:
        capacity = self.point_spin.value()
        for channel in self.channels.values():
            channel.set_capacity(capacity)
        for panel in self.plot_panels:
            panel.mark_dirty()

    def _clear_data(self) -> None:
        self.records = []
        self.line_count = 0
        self.rx_byte_count = 0
        self.rx_byte_label.setText("RX Bytes: 0")
        self.start_time = time.perf_counter()
        self.recent_line_times.clear()
        self._last_capture_index = None
        self._capture_missing_count = 0
        self._capture_received_count = 0
        self._capture_sample_period_seconds = DEFAULT_CAPTURE_SAMPLE_PERIOD_SECONDS
        self._capture_base_time = 0.0
        for channel in self.channels.values():
            channel.times.clear()
            channel.values.clear()
            channel.last_value = None
        for panel in self.plot_panels:
            panel.clear_data()
            panel.update_status(0, 0.0)
        self.raw_log.clear()

    def _export_csv(self) -> None:
        if not self.records:
            QtWidgets.QMessageBox.information(self, "No Data", "No samples to export.")
            return

        default_name = time.strftime("motor_tuning_%Y%m%d_%H%M%S.csv")
        path, _ = QtWidgets.QFileDialog.getSaveFileName(
            self,
            "Export CSV",
            str(Path.cwd() / default_name),
            "CSV Files (*.csv)",
        )
        if not path:
            return

        field_names = ["timestamp"]
        for record in self.records:
            for key in record.keys():
                if key != "timestamp" and key not in field_names:
                    field_names.append(key)

        with open(path, "w", newline="", encoding="utf-8-sig") as csv_file:
            writer = csv.DictWriter(csv_file, fieldnames=field_names)
            writer.writeheader()
            writer.writerows(self.records)

        self.status_label.setText("Exported {}".format(Path(path).name))

    def _send_command_from_index(self, index: int) -> None:
        if index < 0 or index >= len(self.command_inputs):
            return
        command = self.command_inputs[index].text().strip()
        if not command:
            return
        self._send_text_command(command)

    def _send_all_commands(self) -> None:
        if self.worker is None:
            QtWidgets.QMessageBox.information(self, "Not Connected", "Connect serial first.")
            return

        commands = []
        for line_edit in self.command_inputs:
            command = line_edit.text().strip()
            if command:
                commands.append(command)
        if not commands:
            QtWidgets.QMessageBox.information(self, "No Command", "No command to send.")
            return

        interval_ms = self.send_interval_spin.value()
        for idx, command in enumerate(commands):
            QtCore.QTimer.singleShot(interval_ms * idx, self._make_delayed_send_handler(command))

    def _make_delayed_send_handler(self, command: str):
        def _handler():
            if self.worker is None:
                return
            self._send_text_command(command)

        return _handler

    def _send_text_command(self, command: str) -> None:
        if self.worker is None:
            QtWidgets.QMessageBox.information(self, "Not Connected", "Connect serial first.")
            return
        text = command.strip()
        if not text:
            return
        self.worker.enqueue_write(text)
        self._append_log_line(">>> {}".format(text))

    def _send_target_command(self) -> None:
        self._send_text_command("target={:.3f}".format(self.target_speed_spin.value()))

    def _send_pid_command(self) -> None:
        self._send_text_command(
            "pid={:.4f},{:.4f},{:.4f}".format(
                self.kp_spin.value(),
                self.ki_spin.value(),
                self.kd_spin.value(),
            )
        )

    def _send_stream_command(self) -> None:
        enabled = 1 if self.stream_enable_check.isChecked() else 0
        self._send_text_command("stream={}".format(enabled))
        self._send_text_command("period={}".format(self.stream_period_spin.value()))

    def _append_log_line(self, text: str) -> None:
        self.raw_log.appendPlainText(text)

    def _create_channel(self, name: str) -> ChannelSeries:
        color = COLORS[self._color_index % len(COLORS)]
        self._color_index += 1
        channel = ChannelSeries(color=color)
        channel.set_capacity(self.point_spin.value())
        self.channels[name] = channel
        for panel in self.plot_panels:
            panel.ensure_channel(name, channel)
        return channel

    def _current_receive_rate(self) -> float:
        now = time.perf_counter() - self.start_time
        window_duration = max(min(self.rate_window_seconds, now), 0.001)
        return len(self.recent_line_times) / window_duration

    @QtCore.pyqtSlot(list)
    def _handle_lines(self, batch) -> None:
        for raw_line, parsed in batch:
            self._handle_single_line(raw_line, parsed)

    @QtCore.pyqtSlot(int, str)
    def _handle_raw_bytes(self, count: int, preview: str) -> None:
        self.rx_byte_count += count
        self.rx_byte_label.setText("RX Bytes: {}".format(self.rx_byte_count))

        if self.line_count != self._connect_line_count:
            return

        now = time.perf_counter()
        if now - self._last_raw_preview_log >= 1.0:
            self._last_raw_preview_log = now
            self._append_log_line("### RX raw bytes +{} preview: {}".format(count, preview))

    def _handle_single_line(self, raw_line: str, parsed: Dict[str, float]) -> None:
        now = time.perf_counter() - self.start_time
        self.line_count += 1
        self.recent_line_times.append(now)
        cutoff = now - self.rate_window_seconds
        while self.recent_line_times and self.recent_line_times[0] < cutoff:
            self.recent_line_times.popleft()

        log_now = time.perf_counter()
        high_priority_line = (
            raw_line.startswith("ack:")
            or raw_line.startswith("err:")
            or raw_line.startswith("rxcmd:")
            or raw_line.startswith("diag:")
            or raw_line.startswith("boot:")
        )

        if high_priority_line:
            self._append_log_line(raw_line)
        elif (log_now - self._last_line_log_time) >= 0.05:
            self._last_line_log_time = log_now
            self._append_log_line(raw_line)
        if parsed:
            self._update_telemetry_summary(parsed)
            x_value = now
            capture_idx_value = parsed.get("cap_idx")
            if "cap_begin" in parsed:
                self._last_capture_index = None
                self._capture_missing_count = 0
                self._capture_received_count = 0
                self._capture_base_time = now
                cap_dt_us = parsed.get("cap_dt_us")
                if cap_dt_us is not None and cap_dt_us > 0:
                    self._capture_sample_period_seconds = cap_dt_us / 1_000_000.0
                self._append_log_line(
                    "### Capture begin: {} samples".format(int(parsed.get("cap_n", 0)))
                )
            if capture_idx_value is not None:
                capture_index = int(capture_idx_value)
                x_value = self._capture_base_time + capture_index * self._capture_sample_period_seconds
                if self._last_capture_index is not None and capture_index != self._last_capture_index + 1:
                    missed = max(0, capture_index - self._last_capture_index - 1)
                    self._capture_missing_count += missed
                    self._append_log_line(
                        "### Capture sequence gap: expected {}, got {}, missed {}".format(
                            self._last_capture_index + 1,
                            capture_index,
                            missed,
                        )
                    )
                self._last_capture_index = capture_index
                self._capture_received_count += 1
            if "cap_done" in parsed:
                self._append_log_line(
                    "### Capture done: received {}, missing {}".format(
                        self._capture_received_count,
                        self._capture_missing_count,
                    )
                )

            visible_parsed = {
                key: value for key, value in parsed.items() if key in PLOT_CHANNEL_KEYS
            }
            if not visible_parsed:
                rate = self._current_receive_rate()
                for panel in self.plot_panels:
                    panel.update_status(self.line_count, rate)
                return

            record: Dict[str, float] = {"timestamp": round(now, 6)}
            if capture_idx_value is not None:
                record["cap_idx"] = float(capture_idx_value)
            for key, value in visible_parsed.items():
                record[key] = value
                channel = self.channels.get(key)
                if channel is None:
                    channel = self._create_channel(key)
                channel.times.append(x_value)
                channel.values.append(value)
                channel.last_value = value
                for panel in self.plot_panels:
                    panel.set_latest_value(key, value)
            self.records.append(record)
            if len(self.records) > 100000:
                self.records.pop(0)
            for panel in self.plot_panels:
                panel.mark_dirty()

        rate = self._current_receive_rate()
        for panel in self.plot_panels:
            panel.update_status(self.line_count, rate)

    def _update_telemetry_summary(self, parsed: Dict[str, float]) -> None:
        for key, value in parsed.items():
            label = self.telemetry_value_labels.get(key)
            if label is None:
                continue

            if key == "enc_count":
                label.setText(str(int(value)))
            else:
                label.setText("{:.3f}".format(value))

    def _set_status(self, text: str) -> None:
        self.status_label.setText(text)
        self._append_log_line("### {}".format(text))
        if "failed" in text.lower():
            QtWidgets.QMessageBox.warning(self, "Serial Error", text)

    def closeEvent(self, event: QtGui.QCloseEvent) -> None:
        self._disconnect_serial()
        for window in list(self.plot_windows):
            window.close()
        super().closeEvent(event)


def main() -> int:
    app = QtWidgets.QApplication(sys.argv)
    app.setStyle("Fusion")
    app.setStyleSheet(APP_STYLESHEET)
    window = MotorTunerWindow()
    window.show()
    return app.exec_()


if __name__ == "__main__":
    raise SystemExit(main())
