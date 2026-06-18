import sys
import os
import struct
import json
from PyQt5.QtWidgets import (QApplication, QMainWindow, QWidget, QVBoxLayout, 
                            QHBoxLayout, QLabel, QPushButton, QFileDialog, 
                            QMessageBox, QFrame, QDialog, QComboBox,
                            QDialogButtonBox, QFormLayout, QCompleter, QSizePolicy,
                            QLineEdit, QGroupBox, QCheckBox)
from PyQt5.QtCore import Qt, pyqtSignal, QPropertyAnimation, pyqtProperty, QEasingCurve
from PyQt5.QtGui import QDragEnterEvent, QDropEvent, QPalette, QColor, QIcon, QPainter
from intelhex import IntelHex

# --- Config Management ---
if getattr(sys, 'frozen', False):
    CURRENT_DIR = os.path.dirname(sys.executable)
else:
    CURRENT_DIR = os.path.dirname(os.path.abspath(__file__))

PATH_CONFIG_FILE = os.path.join(CURRENT_DIR, "path.json")
DEFAULT_CONFIG_FILENAME = os.path.join(CURRENT_DIR, "config.json")

DEFAULT_CONFIG = {
    "default_dir": r"D:\Desktop\V252x\Software\Bin",
    "app_address": "0x5000",
    "theme": "light",
    "enable_crc": True,
    "crc_address": "0x4E04",
    "font_family": "Segoe UI"
}
 
def resource_path(relative_path):
    """ Get absolute path to resource, works for dev and for PyInstaller """
    try:
        # PyInstaller creates a temp folder and stores path in _MEIPASS
        base_path = sys._MEIPASS
    except Exception:
        base_path = os.path.abspath(".")

    return os.path.join(base_path, relative_path)

def get_config_path():
    if os.path.exists(PATH_CONFIG_FILE):
        try:
            with open(PATH_CONFIG_FILE, 'r', encoding='utf-8') as f:
                data = json.load(f)
                path = data.get("config_path")
                if path:
                    # If path is relative, make it relative to the application directory
                    if not os.path.isabs(path):
                        return os.path.abspath(os.path.join(CURRENT_DIR, path))
                    return path
        except:
            pass
    return os.path.abspath(DEFAULT_CONFIG_FILENAME)

def save_config_path(path):
    # Try to save as relative path if inside main directory for portability
    try:
        abs_path = os.path.abspath(path)
        abs_root = os.path.abspath(CURRENT_DIR)
        
        # Case-insensitive check for Windows mostly, but general enough
        if abs_path.lower().startswith(abs_root.lower()):
            try:
                rel = os.path.relpath(abs_path, abs_root)
                # If relpath doesn't start with dots, it's inside
                if not rel.startswith("..") and not os.path.isabs(rel):
                    path = rel
            except:
                pass
    except:
        pass

    try:
        with open(PATH_CONFIG_FILE, 'w', encoding='utf-8') as f:
            json.dump({"config_path": path}, f, indent=4)
    except Exception as e:
        print(f"Save path config failed: {e}")

def load_config():
    config_file = get_config_path()
    
    # 1. Try to load from the active config path (local or custom)
    if os.path.exists(config_file):
        try:
            with open(config_file, 'r', encoding='utf-8') as f:
                config = json.load(f)
                # Merge with default to ensure all keys exist
                return {**DEFAULT_CONFIG, **config}
        except:
            pass
            
    # 2. If not found, try to load from bundled resource (for EXE)
    bundled_config = resource_path("config.json")
    if os.path.exists(bundled_config) and os.path.abspath(bundled_config) != os.path.abspath(config_file):
        try:
            with open(bundled_config, 'r', encoding='utf-8') as f:
                config = json.load(f)
                return {**DEFAULT_CONFIG, **config}
        except:
            pass
            
    return DEFAULT_CONFIG.copy()

def save_config(config):
    config_file = get_config_path()
    try:
        directory = os.path.dirname(config_file)
        if directory:
            os.makedirs(directory, exist_ok=True)
        with open(config_file, 'w', encoding='utf-8') as f:
            json.dump(config, f, indent=4)
    except Exception as e:
        print(f"Save config failed: {e}")

# --- CRC Logic ---
def calculate_crc16(data, offset=0):
    """
    Calculates CRC16 using the polynomial 0x8408 (XMODEM/CCITT-False reverse).
    Matches the logic from the original merge.py.
    """
    crc_result = 0xA28C
    size = len(data) - offset
    while size != 0:
        crc_result ^= data[offset]
        offset += 1
        for _ in range(8):
            if (crc_result & 0x1) == 0x1:
                crc_result >>= 1
                crc_result ^= 0x8408
            else:
                crc_result >>= 1
        
        size -= 1
    crc_result = (~crc_result) & 0xFFFF
    return crc_result.to_bytes(2, byteorder='little')

# --- Hex Conversion Logic ---
def to_hex_string(byte):
    return f"{byte:02X}"

def calculate_checksum(record):
    return (0x100 - (sum(record) & 0xFF)) & 0xFF

def convert_bin_to_hex(bin_data, start_address):
    """
    Converts binary data to a list of hex records (strings).
    Modified from user request to return list instead of writing file directly,
    to facilitate merging.
    """
    hex_records = []
    address = start_address
    current_segment = (address >> 16) & 0xFFFF  # 初始化当前段地址

    # 添加第一行扩展线性地址记录
    record = [0x02, 0x00, 0x00, 0x04, (current_segment >> 8) & 0xFF, current_segment & 0xFF]
    checksum = calculate_checksum(record)
    record.append(checksum)
    hex_record = ':' + ''.join(to_hex_string(byte) for byte in record)
    hex_records.append(hex_record)

    for i in range(0, len(bin_data), 32):
        # 当前段地址
        segment = (address >> 16) & 0xFFFF
        if segment != current_segment:
            # 如果段地址变化，添加扩展线性地址记录
            current_segment = segment
            record = [0x02, 0x00, 0x00, 0x04, (segment >> 8) & 0xFF, segment & 0xFF]
            checksum = calculate_checksum(record)
            record.append(checksum)
            hex_record = ':' + ''.join(to_hex_string(byte) for byte in record)
            hex_records.append(hex_record)

        chunk = bin_data[i:i+32]
        length = len(chunk)
        record = [length, (address >> 8) & 0xFF, address & 0xFF, 0x00] + list(chunk)
        checksum = calculate_checksum(record)
        record.append(checksum)
        hex_record = ':' + ''.join(to_hex_string(byte) for byte in record)
        hex_records.append(hex_record)
        address += length

    # Note: We do NOT add the End of File record here because we might merge multiple segments.
    # The caller should add ':00000001FF' at the very end.
    return hex_records

class MetaDialog(QDialog):
    def __init__(self, parent=None, theme="light", font_family="Segoe UI"):
        super().__init__(parent)
        self.setWindowTitle("输入文件元信息")
        self.setModal(True)

        # Project
        self.project_combo = QComboBox()
        self.project_combo.setEditable(True)
        self.project_combo.addItems(["V2525C", "V2525D", "V2558", "V2559", "V2593", "V2555", "V2555A", "V2524A", "V2525A", "V2524E", "V2529"])
        proj_completer = QCompleter(self.project_combo.model(), self)
        proj_completer.setCompletionMode(QCompleter.PopupCompletion)
        proj_completer.setCaseSensitivity(Qt.CaseInsensitive)
        self.project_combo.setCompleter(proj_completer)

        # Product Body
        self.product_combo = QComboBox()
        self.product_combo.addItems(["HAND", "Brush", "Bottom", "Battery"])
        self.product_combo.setCurrentText("HAND")

        # Phase
        self.phase_combo = QComboBox()
        self.phase_combo.addItems(["P1", "P2", "EVT", "DVT", "PVT", "MP"])

        # Version
        self.version_combo = QComboBox()
        self.version_combo.setEditable(True)
        self.version_combo.addItems(["V1", "V2", "V3", "V4", "V5"])
        ver_completer = QCompleter(self.version_combo.model(), self)
        ver_completer.setCompletionMode(QCompleter.PopupCompletion)
        ver_completer.setCaseSensitivity(Qt.CaseInsensitive)
        self.version_combo.setCompleter(ver_completer)

        form = QFormLayout()
        form.addRow("项目号:", self.project_combo)
        form.addRow("产品主体:", self.product_combo)
        form.addRow("阶段号:", self.phase_combo)
        form.addRow("版本号:", self.version_combo)

        self.button_box = QDialogButtonBox(QDialogButtonBox.Ok | QDialogButtonBox.Cancel)
        self.button_box.accepted.connect(self.accept)
        self.button_box.rejected.connect(self.reject)

        layout = QVBoxLayout()
        layout.addLayout(form)
        layout.addWidget(self.button_box)
        self.setLayout(layout)
        
        self.apply_theme(theme, font_family)

    def apply_theme(self, theme, font_family="Segoe UI"):
        if theme == "dark":
             self.setStyleSheet(f"""
                QDialog {{ background-color: #2D2D2D; color: #DDD; font-family: '{font_family}', sans-serif; font-size: 10pt; }}
                QLabel {{ color: #DDD; font-size: 14px; }}
                QComboBox {{ 
                    background-color: #3A3A3A; color: #DDD; border: 1px solid #555; border-radius: 6px; padding: 6px; min-width: 200px;
                }}
                QComboBox:focus {{ border: 1px solid #3498db; }}
                QComboBox::drop-down {{
                    subcontrol-origin: padding;
                    subcontrol-position: top right;
                    width: 20px;
                    border-left-width: 0px;
                    border-top-right-radius: 3px;
                    border-bottom-right-radius: 3px;
                }}
                QComboBox::down-arrow {{ 
                    image: none; 
                    border-left: 4px solid transparent;
                    border-right: 4px solid transparent;
                    border-top: 6px solid #DDD;
                    width: 0;
                    height: 0;
                    margin-right: 0px;
                }}
                QPushButton {{ 
                    background-color: #3A3A3A; color: #DDD; border: 1px solid #555; border-radius: 6px; padding: 6px 15px; font-size: 13px;
                }}
                QPushButton:hover {{ background-color: #444; }}
                QPushButton:pressed {{ background-color: #222; }}
            """)
        else:
             self.setStyleSheet(f"""
                QDialog {{ background-color: #FFFFFF; color: #333; font-family: '{font_family}', sans-serif; font-size: 10pt; }}
                QLabel {{ color: #333; font-size: 14px; }}
                QComboBox {{ 
                    background-color: #F9F9F9; color: #333; border: 1px solid #DDD; border-radius: 6px; padding: 6px; min-width: 200px;
                }}
                QComboBox:focus {{ border: 1px solid #3498db; background-color: #FFF; }}
                QComboBox::drop-down {{
                    subcontrol-origin: padding;
                    subcontrol-position: top right;
                    width: 20px;
                    border-left-width: 0px;
                    border-top-right-radius: 3px;
                    border-bottom-right-radius: 3px;
                }}
                QComboBox::down-arrow {{ 
                    image: none; 
                    border-left: 4px solid transparent;
                    border-right: 4px solid transparent;
                    border-top: 6px solid #555;
                    width: 0;
                    height: 0;
                    margin-right: 0px;
                }}
                QPushButton {{ 
                    background-color: #F5F5F5; color: #333; border: 1px solid #DDD; border-radius: 6px; padding: 6px 15px; font-size: 13px;
                }}
                QPushButton:hover {{ background-color: #EAEAEA; }}
                QPushButton:pressed {{ background-color: #DDD; }}
            """)

    def get_values(self):
        return (self.project_combo.currentText().strip(),
                self.product_combo.currentText().strip(),
                self.phase_combo.currentText().strip(),
                self.version_combo.currentText().strip())

class SwitchButton(QWidget):
    stateChanged = pyqtSignal(bool)

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setFixedSize(50, 26)
        self._checked = False
        self._bg_color_off = "#CCC"
        self._bg_color_on = "#2ECC71"
        self._circle_color = "#FFF"
        self.setCursor(Qt.PointingHandCursor)
        
        # Animation setup
        self._circle_position = 3
        self._anim = QPropertyAnimation(self, b"circle_position", self)
        self._anim.setEasingCurve(QEasingCurve.OutCubic)
        self._anim.setDuration(300)

    @pyqtProperty(float)
    def circle_position(self):
        return self._circle_position

    @circle_position.setter
    def circle_position(self, pos):
        self._circle_position = pos
        self.update()

    def isChecked(self):
        return self._checked

    def setChecked(self, checked):
        self._checked = checked
        end_val = self.width() - (self.height() - 6) - 3 if self._checked else 3
        
        if self.isVisible():
            self._anim.stop()
            self._anim.setEndValue(end_val)
            self._anim.start()
        else:
            self._circle_position = end_val
            self.update()
            
        self.stateChanged.emit(checked)

    def mousePressEvent(self, event):
        if event.button() == Qt.LeftButton:
            self.setChecked(not self._checked)

    def paintEvent(self, event):
        p = QPainter(self)
        p.setRenderHint(QPainter.Antialiasing)
        
        rect = self.rect()
        radius = rect.height() - 6
        end_pos = rect.width() - radius - 3
        start_pos = 3
        
        # Calculate progress 0.0 to 1.0
        if end_pos > start_pos:
            progress = (self._circle_position - start_pos) / (end_pos - start_pos)
        else:
            progress = 0.0
            
        progress = max(0.0, min(1.0, progress))
        
        # Interpolate color
        c_off = QColor(self._bg_color_off)
        c_on = QColor(self._bg_color_on)
        
        r = c_off.red() + (c_on.red() - c_off.red()) * progress
        g = c_off.green() + (c_on.green() - c_off.green()) * progress
        b = c_off.blue() + (c_on.blue() - c_off.blue()) * progress
        
        bg_color = QColor(int(r), int(g), int(b))
        
        p.setBrush(bg_color)
        p.setPen(Qt.NoPen)
        p.drawRoundedRect(0, 0, rect.width(), rect.height(), rect.height() / 2, rect.height() / 2)
        
        # Draw Circle
        p.setBrush(QColor(self._circle_color))
        p.drawEllipse(int(self._circle_position), 3, radius, radius)
            
    def set_theme(self, theme):
        if theme == "dark":
            self._bg_color_off = "#555"
            self._circle_color = "#DDD"
        else:
            self._bg_color_off = "#CCC"
            self._circle_color = "#FFF"
        self.update()

class SettingsDialog(QDialog):
    def __init__(self, config, parent=None):
        super().__init__(parent)
        self.config = config
        self.setWindowTitle("设置")
        self.resize(350, 100)
        
        layout = QVBoxLayout()
        layout.setSpacing(8)
        layout.setContentsMargins(15, 15, 15, 15)

        # Config Path
        config_group = QGroupBox("配置文件路径")
        config_layout = QHBoxLayout()
        config_layout.setContentsMargins(8, 8, 8, 8)
        self.config_path_edit = QLineEdit(get_config_path())
        self.config_path_btn = QPushButton("浏览")
        self.config_path_btn.setFixedWidth(50)
        self.config_path_btn.clicked.connect(self.browse_config_path)
        config_layout.addWidget(self.config_path_edit)
        config_layout.addWidget(self.config_path_btn)
        config_group.setLayout(config_layout)
        layout.addWidget(config_group)
        
        # Default Directory
        dir_group = QGroupBox("默认打开路径")
        dir_layout = QHBoxLayout()
        dir_layout.setContentsMargins(8, 8, 8, 8)
        self.dir_edit = QLineEdit(config.get("default_dir", ""))
        self.dir_btn = QPushButton("浏览")
        self.dir_btn.setFixedWidth(50)
        self.dir_btn.clicked.connect(self.browse_dir)
        dir_layout.addWidget(self.dir_edit)
        dir_layout.addWidget(self.dir_btn)
        dir_group.setLayout(dir_layout)
        layout.addWidget(dir_group)
        
        # APP Address
        addr_group = QGroupBox("APP起始地址 (Hex)")
        addr_layout = QHBoxLayout()
        addr_layout.setContentsMargins(8, 8, 8, 8)
        self.addr_edit = QLineEdit(str(config.get("app_address", "0x5000")))
        addr_layout.addWidget(self.addr_edit)
        addr_group.setLayout(addr_layout)
        layout.addWidget(addr_group)

        # CRC Settings
        crc_group = QGroupBox("CRC 设置")
        crc_layout = QVBoxLayout()
        crc_layout.setContentsMargins(8, 8, 8, 8)
        
        # Switch Layout
        switch_layout = QHBoxLayout()
        self.crc_label = QLabel("启用 CRC 校验")
        self.crc_switch = SwitchButton()
        self.crc_switch.setFixedSize(40, 20) # Smaller switch
        self.crc_switch.setChecked(config.get("enable_crc", True))
        self.crc_switch.stateChanged.connect(self.toggle_crc_input)
        
        switch_layout.addWidget(self.crc_label)
        switch_layout.addWidget(self.crc_switch)
        switch_layout.addStretch()
        
        crc_input_layout = QHBoxLayout()
        crc_input_layout.addWidget(QLabel("CRC 地址 (Hex):"))
        self.crc_addr_edit = QLineEdit(str(config.get("crc_address", "0x4E04")))
        crc_input_layout.addWidget(self.crc_addr_edit)
        
        crc_layout.addLayout(switch_layout)
        crc_layout.addLayout(crc_input_layout)
        crc_group.setLayout(crc_layout)
        layout.addWidget(crc_group)
        
        # Initialize state
        self.toggle_crc_input(self.crc_switch.isChecked())
        
        # Appearance (Theme & Font)
        appearance_group = QGroupBox("外观设置")
        appearance_layout = QFormLayout()
        appearance_layout.setContentsMargins(8, 8, 8, 8)
        
        # Theme
        self.theme_combo = QComboBox()
        self.theme_combo.addItems(["Light", "Dark"])
        current_theme = config.get("theme", "light").capitalize()
        index = self.theme_combo.findText(current_theme)
        if index >= 0:
            self.theme_combo.setCurrentIndex(index)
            
        # Font
        self.font_combo = QComboBox()
        self.font_combo.addItems(["Segoe UI", "Microsoft YaHei", "Arial", "SimSun", "Consolas"])
        current_font = config.get("font_family", "Segoe UI")
        font_index = self.font_combo.findText(current_font)
        if font_index >= 0:
            self.font_combo.setCurrentIndex(font_index)
        else:
            self.font_combo.setCurrentIndex(0)

        appearance_layout.addRow("主题模式:", self.theme_combo)
        appearance_layout.addRow("字体风格:", self.font_combo)
        
        appearance_group.setLayout(appearance_layout)
        layout.addWidget(appearance_group)
        
        # Buttons
        btn_box = QDialogButtonBox(QDialogButtonBox.Ok | QDialogButtonBox.Cancel)
        btn_box.accepted.connect(self.accept)
        btn_box.rejected.connect(self.reject)
        layout.addWidget(btn_box)
        
        self.setLayout(layout)
        self.apply_theme(config.get("theme", "light"))
        
    def apply_theme(self, theme):
        self.crc_switch.set_theme(theme)
        font_family = self.config.get("font_family", "Segoe UI")
        
        if theme == "dark":
            self.setStyleSheet(f"""
                QDialog {{ background-color: #2D2D2D; color: #DDD; font-family: '{font_family}', sans-serif; font-size: 8px; }}
                QGroupBox {{ 
                    font-weight: bold; color: #DDD; border: 1px solid #555; border-radius: 8px; margin-top: 6px; padding-top: 6px;
                }}
                QGroupBox::title {{ subcontrol-origin: margin; left: 10px; padding: 0 5px; }}
                QLineEdit {{ 
                    background-color: #3A3A3A; color: #DDD; border: 1px solid #555; border-radius: 6px; padding: 3px; 
                }}
                QLineEdit:disabled {{ background-color: #2A2A2A; color: #777; border: 1px solid #444; }}
                QLineEdit:focus {{ border: 1px solid #3498db; }}
                QLabel {{ color: #DDD; }}
                QPushButton {{ 
                    background-color: #3A3A3A; color: #DDD; border: 1px solid #555; border-radius: 6px; padding: 3px 8px; 
                }}
                QPushButton:hover {{ background-color: #444; border-color: #666; }}
                QPushButton:pressed {{ background-color: #222; }}
                QComboBox {{ 
                    background-color: #3A3A3A; color: #DDD; border: 1px solid #555; border-radius: 6px; padding: 3px; 
                }}
                QComboBox::drop-down {{
                    subcontrol-origin: padding;
                    subcontrol-position: top right;
                    width: 20px;
                    border-left-width: 0px;
                    border-top-right-radius: 3px;
                    border-bottom-right-radius: 3px;
                }}
                QComboBox::down-arrow {{ 
                    image: none; 
                    border-left: 4px solid transparent;
                    border-right: 4px solid transparent;
                    border-top: 6px solid #DDD;
                    width: 0;
                    height: 0;
                    margin-right: 0px;
                }}
            """)
        else:
            self.setStyleSheet(f"""
                QDialog {{ background-color: #FFFFFF; color: #333; font-family: '{font_family}', sans-serif; font-size: 8px; }}
                QGroupBox {{ 
                    font-weight: bold; color: #333; border: 1px solid #DDD; border-radius: 8px; margin-top: 6px; padding-top: 6px;
                }}
                QGroupBox::title {{ subcontrol-origin: margin; left: 10px; padding: 0 5px; }}
                QLineEdit {{ 
                    background-color: #F9F9F9; color: #333; border: 1px solid #DDD; border-radius: 6px; padding: 3px; 
                }}
                QLineEdit:disabled {{ background-color: #EFEFEF; color: #AAA; border: 1px solid #DDD; }}
                QLineEdit:focus {{ border: 1px solid #3498db; background-color: #FFF; }}
                QLabel {{ color: #333; }}
                QPushButton {{ 
                    background-color: #F5F5F5; color: #333; border: 1px solid #DDD; border-radius: 6px; padding: 3px 8px; 
                }}
                QPushButton:hover {{ background-color: #EAEAEA; border-color: #CCC; }}
                QPushButton:pressed {{ background-color: #DDD; }}
                QComboBox {{ 
                    background-color: #F9F9F9; color: #333; border: 1px solid #DDD; border-radius: 6px; padding: 3px; 
                }}
                QComboBox::drop-down {{
                    subcontrol-origin: padding;
                    subcontrol-position: top right;
                    width: 20px;
                    border-left-width: 0px;
                    border-top-right-radius: 3px;
                    border-bottom-right-radius: 3px;
                }}
                QComboBox::down-arrow {{ 
                    image: none; 
                    border-left: 4px solid transparent;
                    border-right: 4px solid transparent;
                    border-top: 6px solid #555;
                    width: 0;
                    height: 0;
                    margin-right: 0px;
                }}
            """)

    def toggle_crc_input(self, state):
        # state is bool from SwitchButton
        self.crc_addr_edit.setEnabled(state)

    def browse_config_path(self):
        file_path, _ = QFileDialog.getOpenFileName(self, "选择配置文件路径", self.config_path_edit.text(), "JSON Files (*.json)")
        if file_path:
            try:
                # Validate first
                with open(file_path, 'r', encoding='utf-8') as f:
                    new_conf = json.load(f)
                
                if not isinstance(new_conf, dict):
                    raise ValueError("Configuration must be a JSON object")

                # If valid, apply settings
                self.config_path_edit.setText(file_path)
                
                if "default_dir" in new_conf:
                    self.dir_edit.setText(new_conf["default_dir"])
                if "app_address" in new_conf:
                    self.addr_edit.setText(str(new_conf["app_address"]))
                if "enable_crc" in new_conf:
                        self.crc_switch.setChecked(new_conf["enable_crc"])
                if "crc_address" in new_conf:
                        self.crc_addr_edit.setText(str(new_conf["crc_address"]))
                if "theme" in new_conf:
                    index = self.theme_combo.findText(new_conf["theme"].capitalize())
                    if index >= 0: self.theme_combo.setCurrentIndex(index)
                if "font_family" in new_conf:
                    index = self.font_combo.findText(new_conf["font_family"])
                    if index >= 0: self.font_combo.setCurrentIndex(index)
                    
            except Exception as e:
                # Show error dialog if invalid
                QMessageBox.warning(self, "无效配置文件", "请加载有效配置文件！\n\n文件内容无法解析或格式不正确。")

    def browse_dir(self):
        directory = QFileDialog.getExistingDirectory(self, "选择默认目录", self.dir_edit.text())
        if directory:
            self.dir_edit.setText(directory)
            
    def get_new_config(self):
        return {
            "config_path": self.config_path_edit.text(),
            "default_dir": self.dir_edit.text(),
            "app_address": self.addr_edit.text(),
            "theme": self.theme_combo.currentText().lower(),
            "enable_crc": self.crc_switch.isChecked(),
            "crc_address": self.crc_addr_edit.text(),
            "font_family": self.font_combo.currentText()
        }

# --- GUI Components ---
class DropArea(QFrame):
    clicked = pyqtSignal()

    def __init__(self, title, color_hex="#f0f0f0"):
        super().__init__()
        self.setAcceptDrops(True)
        self.filepath = None
        self.title = title
        
        self.setFrameStyle(QFrame.StyledPanel | QFrame.Sunken)
        self.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        
        # Layout
        layout = QVBoxLayout()
        layout.setContentsMargins(5, 5, 5, 5)
        layout.setSpacing(5)
        
        self.title_label = QLabel(title)
        self.title_label.setAlignment(Qt.AlignCenter)
        # Style will be set by update_theme
        
        self.status_label = QLabel("拖拽文件到这里\n或点击选择")
        self.status_label.setAlignment(Qt.AlignCenter)
        
        layout.addWidget(self.title_label)
        layout.addWidget(self.status_label)
        self.setLayout(layout)
        
        # Initial Style
        self.update_theme("light")

    def update_theme(self, theme, font_family="Segoe UI"):
        if theme == "dark":
            self.default_style = """
                QFrame {
                    background-color: #2D2D2D;
                    border: 2px dashed #555555;
                    border-radius: 12px;
                }
                QFrame:hover {
                    border-color: #3498db;
                    background-color: #383838;
                }
            """
            self.active_style = """
                QFrame {
                    background-color: #383838;
                    border: 2px dashed #3498db;
                    border-radius: 12px;
                }
            """
            self.title_label.setStyleSheet(f"font-family: '{font_family}', sans-serif; font-weight: bold; font-size: 16px; color: #DDDDDD;")
            self.status_label.setStyleSheet(f"font-family: '{font_family}', sans-serif; font-size: 13px; color: #AAAAAA;")
        else:
            self.default_style = """
                QFrame {
                    background-color: #FFFFFF;
                    border: 2px dashed #CCCCCC;
                    border-radius: 12px;
                }
                QFrame:hover {
                    border-color: #3498db;
                    background-color: #F8F9FA;
                }
            """
            self.active_style = """
                QFrame {
                    background-color: #EBF5FB;
                    border: 2px dashed #3498db;
                    border-radius: 12px;
                }
            """
            self.title_label.setStyleSheet(f"font-family: '{font_family}', sans-serif; font-weight: bold; font-size: 16px; color: #333;")
            self.status_label.setStyleSheet(f"font-family: '{font_family}', sans-serif; font-size: 13px; color: #888;")
            
        self.setStyleSheet(self.default_style)

    def dragEnterEvent(self, event: QDragEnterEvent):
        if event.mimeData().hasUrls():
            event.acceptProposedAction()
            self.setStyleSheet(self.active_style)

    def dragLeaveEvent(self, event):
        self.setStyleSheet(self.default_style)

    def dropEvent(self, event: QDropEvent):
        self.setStyleSheet(self.default_style)
        if event.mimeData().hasUrls():
            url = event.mimeData().urls()[0]
            path = url.toLocalFile()
            if path.lower().endswith('.bin'):
                self.set_file(path)
            else:
                QMessageBox.warning(self, "无效文件", "请选择 .bin 文件")

    def mousePressEvent(self, event):
        if event.button() == Qt.LeftButton:
            self.clicked.emit()
            
    def set_file(self, path):
        self.filepath = path
        self.status_label.setText(os.path.basename(path))
        # Keep text color consistent with theme
        # self.status_label.setStyleSheet("color: #000; font-weight: bold;") # Removed to respect theme
        font = self.status_label.font()
        font.setBold(True)
        self.status_label.setFont(font)

class MergeTool(QMainWindow):
    def __init__(self):
        super().__init__()
        self.config = load_config()
        self.setWindowTitle("X系列专用合并bin文件工具")
        self.resize(900, 500)
        
        # Set Icon
        icon_path = resource_path("icon.ico")
        if os.path.exists(icon_path):
            self.setWindowIcon(QIcon(icon_path))
        else:
            icon_path_png = resource_path("icon.png")
            if os.path.exists(icon_path_png):
                self.setWindowIcon(QIcon(icon_path_png))
        
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        layout = QVBoxLayout(central_widget)
        layout.setContentsMargins(10, 10, 10, 10)
        layout.setSpacing(10)
        
        # Drop Areas
        drop_layout = QHBoxLayout()
        drop_layout.setSpacing(10)
        self.area_a = DropArea("Boot 文件 (地址 0x0000)")
        self.area_b = DropArea("APP 文件")
        
        self.area_a.clicked.connect(lambda: self.open_file_dialog(self.area_a))
        self.area_b.clicked.connect(lambda: self.open_file_dialog(self.area_b))
        
        drop_layout.addWidget(self.area_a)
        drop_layout.addWidget(self.area_b)
        
        layout.addLayout(drop_layout, 1)
        
        # Info Area (Label + Settings)
        info_layout = QHBoxLayout()
        info_layout.setSpacing(10)
        
        self.info_label = QLabel("说明: 将自动计算文件B的CRC16并写入 \n合并后将生成 .bin 和 .hex 文件")
        self.info_label.setAlignment(Qt.AlignCenter)
        info_layout.addWidget(self.info_label, 1)
        
        self.btn_settings = QPushButton("⚙")
        self.btn_settings.setFixedSize(40, 40)
        self.btn_settings.setCursor(Qt.PointingHandCursor)
        self.btn_settings.clicked.connect(self.open_settings)
        info_layout.addWidget(self.btn_settings)
        
        layout.addLayout(info_layout)
        
        # Merge Button
        self.btn_merge = QPushButton("执行合并 (Merge)")
        self.btn_merge.setMinimumHeight(50)
        self.btn_merge.setCursor(Qt.PointingHandCursor)
        self.btn_merge.clicked.connect(self.process_merge)
        layout.addWidget(self.btn_merge)
        
        # Apply Theme
        self.apply_theme()

    def apply_theme(self):
        theme = self.config.get("theme", "light")
        font_family = self.config.get("font_family", "Segoe UI")
        self.area_a.update_theme(theme, font_family)
        self.area_b.update_theme(theme, font_family)
        
        if theme == "dark":
            self.setStyleSheet(f"""
                QMainWindow {{ background-color: #1E1E1E; }}
                QLabel {{ font-family: '{font_family}', sans-serif; color: #DDDDDD; }}
                QMessageBox {{ font-family: '{font_family}', sans-serif; background-color: #2D2D2D; color: #DDDDDD; }}
                QMessageBox QPushButton {{
                    background-color: #3A3A3A;
                    color: #DDDDDD;
                    border: 1px solid #555;
                    border-radius: 6px;
                    padding: 6px 15px;
                    min-width: 60px;
                }}
                QMessageBox QPushButton:hover {{ background-color: #444; }}
                QPushButton {{
                    background-color: #3A3A3A;
                    color: #DDDDDD;
                    border: 1px solid #555;
                    border-radius: 6px;
                    padding: 5px;
                }}
                QPushButton:hover {{ background-color: #444; }}
                QComboBox {{
                    background-color: #3A3A3A;
                    color: #DDDDDD;
                    border: 1px solid #555;
                    border-radius: 6px;
                    padding: 5px;
                }}
                QComboBox::drop-down {{
                    subcontrol-origin: padding;
                    subcontrol-position: top right;
                    width: 20px;
                    border-left-width: 0px;
                    border-top-right-radius: 3px;
                    border-bottom-right-radius: 3px;
                }}
                QComboBox::down-arrow {{
                    image: none;
                    border-left: 4px solid transparent;
                    border-right: 4px solid transparent;
                    border-top: 6px solid #DDD;
                    width: 0;
                    height: 0;
                    margin-top: 0px;
                    margin-left: 0px;
                }}
            """)
            self.btn_settings.setStyleSheet("""
                QPushButton {
                    background-color: transparent;
                    color: #DDDDDD;
                    font-size: 24px;
                    border: none;
                }
                QPushButton:hover { color: #FFFFFF; }
            """)
            self.info_label.setStyleSheet(f"""
                color: #AAAAAA; 
                font-family: '{font_family}', sans-serif;
                font-size: 13px;
                background-color: #2D2D2D;
                border-radius: 8px;
                padding: 10px;
                border: 1px solid #333;
            """)
            self.btn_merge.setStyleSheet("""
                QPushButton {
                    background-color: #2ECC71;
                    color: white;
                    font-weight: bold;
                    font-size: 16px;
                    border-radius: 8px;
                    border: none;
                }
                QPushButton:hover { background-color: #27AE60; }
                QPushButton:pressed { background-color: #229954; }
            """)
        else:
            self.setStyleSheet(f"""
                QMainWindow {{ background-color: #F5F6FA; }}
                QLabel {{ font-family: '{font_family}', sans-serif; color: #333; }}
                QMessageBox {{ font-family: '{font_family}', sans-serif; background-color: #FFFFFF; color: #333; }}
                QMessageBox QPushButton {{
                    background-color: #F5F5F5;
                    color: #333;
                    border: 1px solid #DDD;
                    border-radius: 6px;
                    padding: 6px 15px;
                    min-width: 60px;
                }}
                QMessageBox QPushButton:hover {{ background-color: #EAEAEA; }}
                QPushButton {{
                    background-color: #FFFFFF;
                    color: #333;
                    border: 1px solid #CCC;
                    border-radius: 6px;
                    padding: 5px;
                }}
                QPushButton:hover {{ background-color: #F0F0F0; }}
                QComboBox {{
                    background-color: #FFFFFF;
                    color: #333;
                    border: 1px solid #CCC;
                    border-radius: 6px;
                    padding: 5px;
                }}
                QComboBox::drop-down {{
                    subcontrol-origin: padding;
                    subcontrol-position: top right;
                    width: 20px;
                    border-left-width: 0px;
                    border-top-right-radius: 3px;
                    border-bottom-right-radius: 3px;
                }}
                QComboBox::down-arrow {{
                    image: none;
                    border-left: 4px solid transparent;
                    border-right: 4px solid transparent;
                    border-top: 6px solid #555;
                    width: 0;
                    height: 0;
                    margin-top: 0px;
                    margin-left: 0px;
                }}
            """)
            self.btn_settings.setStyleSheet("""
                QPushButton {
                    background-color: transparent;
                    color: #555;
                    font-size: 24px;
                    border: none;
                }
                QPushButton:hover { color: #333; }
            """)
            self.info_label.setStyleSheet(f"""
                color: #555; 
                font-family: '{font_family}', sans-serif;
                font-size: 13px;
                background-color: #FFFFFF;
                border-radius: 8px;
                padding: 10px;
                border: 1px solid #E0E0E0;
            """)
            self.btn_merge.setStyleSheet("""
                QPushButton {
                    background-color: #2ECC71;
                    color: white;
                    font-weight: bold;
                    font-size: 16px;
                    border-radius: 8px;
                    border: none;
                }
                QPushButton:hover { background-color: #27AE60; }
                QPushButton:pressed { background-color: #229954; }
            """)
            
        # Update Address Label
        app_addr_str = self.config.get("app_address", "0x5000")
        self.area_b.title_label.setText(f"APP 文件 (地址 {app_addr_str})")

    def open_settings(self):
        self.config = load_config()
        dlg = SettingsDialog(self.config, self)
        if dlg.exec_() == QDialog.Accepted:
            new_config = dlg.get_new_config()
            
            # Handle config path change
            new_path = new_config.pop("config_path", None)
            if new_path:
                current_path = get_config_path()
                # Normalize paths for comparison
                if os.path.abspath(new_path) != os.path.abspath(current_path):
                    save_config_path(new_path)
            
            self.config = new_config
            save_config(self.config)
            self.apply_theme()

    def open_file_dialog(self, area):
        default_dir = self.config.get("default_dir", "")
        if not os.path.exists(default_dir):
            default_dir = ""
        path, _ = QFileDialog.getOpenFileName(self, f"选择 {area.title}", default_dir, "Bin Files (*.bin)")
        if path:
            area.set_file(path)

    def process_merge(self):
        path_a = self.area_a.filepath
        path_b = self.area_b.filepath
        
        if not path_a or not path_b:
            QMessageBox.warning(self, "提示", "请先加载两个文件！")
            return
            
        try:
            # Get Addresses from Config
            try:
                app_addr = int(self.config.get("app_address", "0x5000"), 16)
            except ValueError:
                QMessageBox.critical(self, "配置错误", "APP起始地址格式错误，请在设置中修正 (例如 0x5000)")
                return
                
            # CRC Logic
            enable_crc = self.config.get("enable_crc", True)
            crc_addr = 0
            crc_hex = ""
            crc_bytes = b''
            
            if enable_crc:
                try:
                    crc_addr_str = self.config.get("crc_address", "0x4E04")
                    crc_addr = int(crc_addr_str, 16)
                except ValueError:
                    QMessageBox.critical(self, "配置错误", "CRC地址格式错误，请在设置中修正 (例如 0x4E04)")
                    return
            
            # 1. Read Files (Binary)
            with open(path_a, 'rb') as f:
                data_a = f.read()
            with open(path_b, 'rb') as f:
                data_b = f.read()

            # CRC value is always calculated for naming, regardless of write switch
            crc_bytes = calculate_crc16(data_b)
            crc_val = int.from_bytes(crc_bytes, byteorder='little')
            crc_hex = f"{crc_val:04X}"
                
            # Check size of A to ensure it doesn't overlap CRC area (only if CRC is enabled)
            if enable_crc:
                if len(data_a) > crc_addr:
                    QMessageBox.critical(self, "错误", f"文件 A 太大 ({len(data_a)} bytes)，覆盖了 CRC 地址 0x{crc_addr:X}！")
                    return
            
            # 3. Get Meta Info
            meta_dlg = MetaDialog(self, self.config.get("theme", "light"), self.config.get("font_family", "Segoe UI"))
            if meta_dlg.exec_() != QDialog.Accepted:
                return
            
            project, product, phase, version = meta_dlg.get_values()
            if not project or not product or not phase or not version:
                 QMessageBox.warning(self, "信息不完整", "请填写完整信息")
                 return

            default_filename = f"{project}_{product}_{phase}_{version}_CRC_{crc_hex}.bin"
            
            # Use File B's directory as default save location
            b_dir = os.path.dirname(path_b)
            if b_dir:
                default_save_path = os.path.join(b_dir, default_filename)
            else:
                default_save_path = default_filename

            # 4. Convert to Hex separately (using custom function)
            hex_records_a = convert_bin_to_hex(data_a, 0x0000)
            hex_records_b = convert_bin_to_hex(data_b, app_addr)
            
            # 5. Merge Hex Records
            if enable_crc:
                hex_records_crc = convert_bin_to_hex(crc_bytes, crc_addr)
                final_hex_records = hex_records_a + hex_records_crc + hex_records_b
            else:
                final_hex_records = hex_records_a + hex_records_b
                
            final_hex_records.append(':00000001FF')
            
            # 6. Save Dialog
            save_path, _ = QFileDialog.getSaveFileName(self, "保存合并文件", default_save_path, "Bin Files (*.bin)")
            if not save_path:
                return
                
            base_path = os.path.splitext(save_path)[0]
            bin_path = base_path + ".bin"
            hex_path = base_path + ".hex"
            
            # 7. Write .hex file
            with open(hex_path, 'w') as f:
                f.write('\n'.join(final_hex_records) + '\n')
            
            # 8. Write .bin file (Reconstruct bytearray for correctness)
            # We use a bytearray to simulate the memory space
            total_size = app_addr + len(data_b)
            merged_data = bytearray([0xFF] * total_size)
            merged_data[0:len(data_a)] = data_a
            if enable_crc:
                merged_data[crc_addr:crc_addr+2] = crc_bytes
            merged_data[app_addr:app_addr+len(data_b)] = data_b
            
            with open(bin_path, 'wb') as f:
                f.write(merged_data)
            
            # 9. Rename File B
            msg_extra = ""
            try:
                new_b_name = f"APP_{project}_{product}_{phase}_{version}_CRC_{crc_hex}.bin"
                new_b_path = os.path.join(b_dir, new_b_name)
                
                if os.path.abspath(path_b) != os.path.abspath(new_b_path):
                    if os.path.exists(new_b_path):
                        reply = QMessageBox.question(self, "文件已存在", 
                                                   f"目标文件 {new_b_name} 已存在，是否覆盖？", 
                                                   QMessageBox.Yes | QMessageBox.No)
                        if reply == QMessageBox.Yes:
                            os.replace(path_b, new_b_path)
                            msg_extra = f"\n\nB文件已重命名为:\n{new_b_name}"
                            self.area_b.filepath = new_b_path
                            self.area_b.status_label.setText(new_b_name)
                        else:
                            msg_extra = "\n\nB文件未重命名 (用户取消)"
                    else:
                        os.rename(path_b, new_b_path)
                        msg_extra = f"\n\nB文件已重命名为:\n{new_b_name}"
                        self.area_b.filepath = new_b_path
                        self.area_b.status_label.setText(new_b_name)
            except Exception as e_rename:
                msg_extra = f"\n\nB文件重命名失败:\n{str(e_rename)}"

            QMessageBox.information(self, "成功", f"合并完成！\n\nBin: {bin_path}\nHex: {hex_path}{msg_extra}")
            
        except Exception as e:
            QMessageBox.critical(self, "错误", f"发生错误:\n{str(e)}")

if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = MergeTool()
    window.show()
    sys.exit(app.exec_())
