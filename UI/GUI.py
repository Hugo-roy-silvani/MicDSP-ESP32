import sys
import serial
import threading
from PyQt6.QtWidgets import (
    QApplication, QWidget, QVBoxLayout, QLabel, QSlider, QPushButton,
    QHBoxLayout, QMessageBox, QTabWidget, QGridLayout, QDoubleSpinBox
)
from PyQt6.QtCore import Qt, QTimer, pyqtSignal
import pyqtgraph as pg


class DSPGUI(QWidget):
    rms_updated = pyqtSignal(float)
    fft_updated = pyqtSignal(list)

    def __init__(self, port="/dev/ttyUSB0", baudrate=115200):
        super().__init__()
        self.setWindowTitle("ESP32 DSP Controller")
        self.resize(800, 600)

        # === Serial setup ===
        try:
            self.ser = serial.Serial(port, baudrate, timeout=1)
        except serial.SerialException:
            QMessageBox.critical(self, "Erreur", f"Impossible d’ouvrir le port série {port}")
            sys.exit(1)

        # === Main layout ===
        layout = QVBoxLayout()
        self.tabs = QTabWidget()
        layout.addWidget(self.tabs)
        self.setLayout(layout)

        # === Tabs ===
        self.tabs.addTab(self.make_eq_tab(), "EQ 3 Bandes")
        self.tabs.addTab(self.make_expander_tab(), "Expander")
        self.tabs.addTab(self.make_compressor_tab(), "Compressor")
        self.tabs.addTab(self.make_limiter_tab(), "Limiter")
        self.tabs.addTab(self.make_monitor_tab(), "Monitoring")

        # === Connect signals ===
        self.rms_updated.connect(self.update_rms_label)
        self.fft_updated.connect(self.update_fft_plot)

        # === Serial listening thread ===
        self.listen_thread = threading.Thread(target=self.listen_serial, daemon=True)
        self.listen_thread.start()

    # ============================================================
    # -------------------- EQ TAB -------------------------------
    # ============================================================
    def _sync_set_spin(self, spin, value: float):
        spin.blockSignals(True)
        try:
            spin.setValue(value)
        finally:
            spin.blockSignals(False)

    def _sync_set_slider_raw(self, slider, raw: int):
        slider.blockSignals(True)
        try:
            slider.setValue(raw)
        finally:
            slider.blockSignals(False)

    def send_eq_q(self, band, value, label, unit, slider=None, spin=None):
        value = max(0.1, min(10.0, float(value)))
        if slider is not None:
            self._sync_set_slider_raw(slider, int(round(value * 1000)))
        if spin is not None:
            self._sync_set_spin(spin, value)
        label.setText(f"Q = {value:.3f} {unit}")
        self.send_cmd(f"EQ_{band}_Q={value:.3f}\n")
    
    
    def make_eq_tab(self):
        tab = QWidget()
        grid = QGridLayout()

        bands = ["LOW", "MID", "HIGH"]
        params = [
            ("fc", 20, 20000, 1000, "Hz"),
            ("Q", 0.1, 10.0, 1.0, ""),
            ("gain", -12, 12, 0, "dB"),
        ]

        row = 0
        for b in bands:
            title = QLabel(f"Band: {b}")
            grid.addWidget(title, row, 0)
            row += 1

            for (p, vmin, vmax, vinit, unit) in params:
                label = QLabel(f"{p.upper()} = {vinit} {unit}")
                if p == "Q":
                    q_slider = QSlider(Qt.Orientation.Horizontal)
                    q_slider.setMinimum(int(vmin * 1000))
                    q_slider.setMaximum(int(vmax * 1000))
                    q_slider.setValue(int(vinit * 1000))
                    q_slider.setTracking(False)

                    q_spin = QDoubleSpinBox()
                    q_spin.setDecimals(3)
                    q_spin.setRange(vmin, vmax)
                    q_spin.setSingleStep(0.001)
                    q_spin.setKeyboardTracking(False)
                    q_spin.setValue(vinit)

                    q_slider.valueChanged.connect(
                        lambda raw, band=b, lbl=label, u=unit, sp=q_spin:
                            self.send_eq_q(band, raw / 1000.0, lbl, u, slider=None, spin=sp)
                    )
                    q_spin.valueChanged.connect(
                        lambda v, band=b, lbl=label, u=unit, sl=q_slider, sp=q_spin:
                            self.send_eq_q(band, float(v), lbl, u, slider=sl, spin=None)
                    )

                    grid.addWidget(label, row, 0)
                    grid.addWidget(q_slider, row, 1, 1, 2)
                    grid.addWidget(q_spin, row, 3)
                    row += 1
                else:
                    slider = QSlider(Qt.Orientation.Horizontal)
                    slider.setMinimum(int(vmin * 10))
                    slider.setMaximum(int(vmax * 10))
                    slider.setValue(int(vinit * 10))
                    slider.valueChanged.connect(
                        lambda val, band=b, param=p, lbl=label, u=unit:
                            self.send_eq_param(band, param, val / 10.0, lbl, u)
                    )
                    grid.addWidget(label, row, 0)
                    grid.addWidget(slider, row, 1, 1, 3)
                    row += 1

        tab.setLayout(grid)
        return tab

    def send_eq_param(self, band, param, value, label, unit):
        label.setText(f"{param.upper()} = {value:.2f} {unit}")
        cmd = f"EQ_{band}_{param.upper()}={value:.2f}\n"
        self.send_cmd(cmd)

    # ============================================================
    # -------------------- EXPANDER / COMP / LIMITER -------------
    # ============================================================
    def make_expander_tab(self):
        return self.make_block_tab("EXPANDER", [
            ("THRESHOLD", 0.0, 1.0, 0.02),
            ("RATIO", 1.0, 10.0, 2.0),
            ("ATTACK", 0.0, 0.1, 0.01),
            ("RELEASE", 0.01, 0.3, 0.1),
            ("HOLD", 0.0, 1.0, 0.1)
        ])

    def make_compressor_tab(self):
        return self.make_block_tab("COMP", [
            ("THRESHOLD", 0.0, 1.0, 0.3),
            ("RATIO", 1.0, 10.0, 2.0),
            ("MAKEUP", 0.0, 2.0, 1.0),
            ("ATTACK", 0.0, 0.1, 0.01),
            ("RELEASE", 0.01, 0.3, 0.1),
            ("KNEE", 0.0, 12.0, 6.0)
        ])

    def make_limiter_tab(self):
        return self.make_block_tab("LIMIT", [
            ("THRESHOLD", 0.0, 1.0, 0.9),
            ("ATTACK", 0.0, 0.1, 0.01),
            ("RELEASE", 0.01, 0.3, 0.1)
        ])

    def make_block_tab(self, name, sliders):
        tab = QWidget()
        layout = QVBoxLayout()
        for param, vmin, vmax, vinit in sliders:
            label = QLabel(f"{param} = {vinit}")
            slider = QSlider(Qt.Orientation.Horizontal)
            slider.setMinimum(int(vmin * 100))
            slider.setMaximum(int(vmax * 100))
            slider.setValue(int(vinit * 100))
            slider.valueChanged.connect(
                lambda val, n=param, lbl=label, block=name:
                    self.send_param(block, n, val / 100.0, lbl))
            layout.addWidget(label)
            layout.addWidget(slider)
        tab.setLayout(layout)
        return tab

    # ============================================================
    # -------------------- MONITOR TAB ---------------------------
    # ============================================================
    def make_monitor_tab(self):
        tab = QWidget()
        layout = QVBoxLayout()

        self.rms_label = QLabel("RMS: 0.0 dBFS")
        layout.addWidget(self.rms_label)

        # FFT graph configuration
        self.fft_plot = pg.PlotWidget(title="Spectre FFT (8 bandes)")
        self.fft_plot.setYRange(-80, 0)
        self.fft_plot.setLabel('left', 'Amplitude (dB)')
        self.fft_plot.setLabel('bottom', 'Bandes')
        self.fft_plot.showGrid(x=True, y=True)

        # 8 band create
        self.fft_x = list(range(8))
        self.fft_bar = pg.BarGraphItem(x=self.fft_x, height=[-80]*8, width=0.8, brush='cyan')
        self.fft_plot.addItem(self.fft_bar)
        layout.addWidget(self.fft_plot)

        tab.setLayout(layout)
        return tab


    def update_rms_label(self, val):
        self.rms_label.setText(f"RMS: {val:.1f} dBFS")

    def update_fft_plot(self, bands):
       
        if len(bands) != 8:
            print(" Données FFT inattendues :", bands)
            return

        # erase old graph and draw new one
        self.fft_plot.clear()
        self.fft_bar = pg.BarGraphItem(
            x=self.fft_x,
            height=bands,
            width=0.8,
            brush='cyan'
        )
        self.fft_plot.addItem(self.fft_bar)


    # ============================================================
    # -------------------- SERIAL COMM ---------------------------
    # ============================================================
    def send_param(self, block, name, value, label):
        label.setText(f"{name} = {value:.2f}")
        cmd = f"{block}_{name}={value:.2f}\n"
        self.send_cmd(cmd)

    def send_cmd(self, cmd):
        self.ser.write(cmd.encode())
        print("→", cmd.strip())

    def listen_serial(self):
        """Listen esp32 msg"""
        while True:
            try:
                line = self.ser.readline().decode(errors="ignore").strip()
                if not line:
                    continue
                print("←", line)

                if line.startswith("STREAM:"):
                    line = line.replace("STREAM:", "")
                    # Get RMS
                    rms_val = None
                    if "RMS=" in line:
                        try:
                            rms_str = line.split("RMS=")[1].split(",")[0]
                            rms_val = float(rms_str)
                            self.rms_updated.emit(rms_val)
                        except ValueError:
                            pass

                    # Get FFT
                    if "FFT=" in line:
                        try:
                            fft_str = line.split("FFT=")[1]
                            fft_vals = [float(x) for x in fft_str.split(",") if x.strip()]
                            if len(fft_vals) == 8:
                                self.fft_updated.emit(fft_vals)
                            else:
                                print("wrong fft data :", fft_vals)
                        except ValueError:
                            print("wrong fft parsing :", line)


            except serial.SerialException:
                break


if __name__ == "__main__":
    port = "/dev/ttyUSB0"  
    app = QApplication(sys.argv)
    gui = DSPGUI(port)
    gui.show()
    sys.exit(app.exec())
