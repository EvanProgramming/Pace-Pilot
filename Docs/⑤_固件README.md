# BeatSync 固件 README

| 项目代号 | BeatSync |
|---------|----------|
| 版本 | v1.0 |
| 日期 | 2026/07/15 |

---

## 1. 项目简介

BeatSync 固件分为两个独立工程：

| 工程 | 目录 | 目标平台 | 开发框架 |
|------|------|---------|---------|
| 耳机端固件 | `src/earphone/` | Seeed XIAO ESP32C3 | Arduino IDE / PlatformIO |
| 戒指端固件 | `src/ring/` | Seeed XIAO nRF52840 | Arduino IDE / PlatformIO |

---

## 2. 开发环境

### 2.1 耳机端（ESP32C3）

| 项目 | 版本/配置 |
|------|----------|
| Arduino IDE | ≥ 2.1.0 |
| ESP32 Board Package | ≥ 2.0.14 (支持 ESP32C3) |
| PlatformIO Core | ≥ 6.1.0（可选） |
| 上传速度 | 921600 baud |
| Flash Size | 4MB |

**依赖库**:
- `ICM42688` (by SparkFun) — IMU 驱动
- `DFRobotDFPlayerMini` — DFPlayer 控制
- `BLEDevice` (ESP32 内置) — BLE 通信

### 2.2 戒指端（nRF52840）

| 项目 | 版本/配置 |
|------|----------|
| Arduino IDE | ≥ 2.1.0 |
| Seeed nRF52 Board Package | ≥ 1.1.2 |
| 上传速度 | 115200 baud |
| SoftDevice | S140 |

**依赖库**:
- `ICM42688` (by SparkFun) — IMU 驱动
- `BLEPeripheral` 或 `bluefruit` (Adafruit) — BLE 通信

### 2.3 安装步骤

```bash
# 方式一：PlatformIO（推荐）
pip install platformio
cd src/earphone
pio run -t upload

# 方式二：Arduino IDE
# 1. 安装对应 Board Package
# 2. 通过 Library Manager 安装依赖库
# 3. 选择正确的开发板和端口
# 4. 上传
```

---

## 3. 代码结构

```
src/
├── earphone/                 # 耳机端固件
│   ├── platformio.ini        # PlatformIO 配置
│   ├── main.cpp              # 主程序入口
│   ├── cadence.h             # 步频检测模块
│   ├── cadence.cpp
│   ├── player.h              # DFPlayer 控制模块
│   ├── player.cpp
│   ├── ble_central.h         # BLE 主机（连接戒指端）
│   ├── ble_central.cpp
│   ├── bpm_matcher.h         # 步频匹配切歌逻辑
│   ├── bpm_matcher.cpp
│   ├── buttons.h             # 按键处理
│   ├── buttons.cpp
│   └── config.h              # 全局配置（引脚、参数）
│
├── ring/                     # 戒指端固件
│   ├── platformio.ini        # PlatformIO 配置
│   ├── main.cpp              # 主程序入口
│   ├── gesture.h             # 手势识别模块（DTW）
│   ├── gesture.cpp
│   ├── ble_peripheral.h      # BLE 从机（广播+通知）
│   ├── ble_peripheral.cpp
│   └── config.h              # 全局配置（引脚、参数）
│
├── tools/                    # 工具脚本
│   ├── bpm_tagger.py         # 音乐文件 BPM 标注工具
│   └── gesture_trainer.py    # 手势模板采集工具
│
└── README.md                 # 本文件
```

---

## 4. 核心模块说明

### 4.1 步频检测（cadence.h/cpp）

```cpp
// 核心 API
class CadenceDetector {
public:
    void begin();                    // 初始化 IMU
    void update();                   // 主循环调用，读取数据
    float getBPM();                  // 获取当前步频
    bool isRunning();                // 是否在跑步
};
```

- 采样率：100Hz
- 滤波：带通 0.5~5Hz
- 窗口：2 秒滑动窗口
- 更新频率：1Hz

### 4.2 手势识别（gesture.h/cpp）

```cpp
// 核心 API
class GestureRecognizer {
public:
    void begin();                    // 初始化 IMU
    void update();                   // 主循环调用
    uint8_t getGesture();            // 获取识别结果
    // 手势 ID: 1=单击 2=双击 3=上滑 4=下滑 5=旋转
    void loadTemplates();            // 加载预存模板
    void recordTemplate(uint8_t id); // 录制新模板
};
```

- 算法：DTW（动态时间规整）
- 录制窗口：1.5 秒
- 触发阈值：加速度能量 > 阈值

### 4.3 DFPlayer 控制（player.h/cpp）

```cpp
// 核心 API
class MusicPlayer {
public:
    void begin();                    // 初始化 DFPlayer
    void play(uint16_t track);       // 播放指定曲目
    void next();                     // 下一首
    void prev();                     // 上一首
    void pause();                    // 暂停
    void setVolume(uint8_t vol);     // 音量 0~30
    uint16_t getCurrentTrack();      // 当前曲目号
};
```

- 通信：UART 9600 baud
- 音量范围：0~30

### 4.4 BLE 通信

**耳机端（ble_central.h/cpp）**:
```cpp
class BLECentral {
public:
    void begin();                    // 初始化 BLE
    void scanAndConnect();           // 扫描并连接戒指端
    bool isConnected();              // 连接状态
    void onGesture(callback);        // 注册手势回调
    void sendHeartbeat();            // 发送心跳
};
```

**戒指端（ble_peripheral.h/cpp）**:
```cpp
class BLEPeripheral {
public:
    void begin();                    // 初始化 BLE 广播
    void notifyGesture(uint8_t id, uint8_t conf);  // 发送手势通知
    void notifyBattery(uint8_t level);              // 发送电量
    bool isConnected();              // 连接状态
};
```

### 4.5 步频匹配切歌（bpm_matcher.h/cpp）

```cpp
class BPMMatcher {
public:
    void begin();                    // 加载歌曲 BPM 表
    void update(float cadenceBPM);   // 输入实时步频
    uint16_t getMatchedTrack();      // 获取匹配曲目号
    bool shouldSwitch();             // 是否需要切歌
};
```

- 切歌条件：步频与当前歌曲 BPM 差值 > ±10 BPM，持续 10 秒
- 歌曲库：microSD 根目录 `bpm_list.csv`

**bpm_list.csv 格式**:
```csv
track_num,filename,bpm
1,run_140_001.mp3,140
2,run_145_002.mp3,145
3,run_150_003.mp3,150
...
```

---

## 5. 配置说明（config.h）

### 5.1 耳机端配置

```cpp
// === 引脚定义 ===
#define PIN_I2C_SDA     2
#define PIN_I2C_SCL     3
#define PIN_UART_RX     4
#define PIN_UART_TX     5
#define PIN_BTN_MODE    6
#define PIN_BTN_VOL_UP  7
#define PIN_BTN_VOL_DN  10
#define PIN_CHG_STATUS  20
#define PIN_LED         21
#define PIN_BAT_SENSE   1

// === IMU 配置 ===
#define IMU_SAMPLE_RATE   100   // Hz
#define IMU_I2C_ADDR      0x69

// === 步频检测配置 ===
#define CADENCE_WINDOW_MS   2000  // 滑动窗口
#define CADENCE_UPDATE_MS   1000  // 更新频率
#define CADENCE_FILTER_LO   0.5   // 带通下限 Hz
#define CADENCE_FILTER_HI   5.0   // 带通上限 Hz

// === DFPlayer 配置 ===
#define DFPLAYER_BAUD     9600
#define DFPLAYER_VOL_MAX  30

// === BPM 匹配配置 ===
#define BPM_MATCH_THRESHOLD  10   // BPM 差值阈值
#define BPM_MATCH_DURATION   10000 // 持续时间 ms

// === BLE 配置 ===
#define BLE_SERVICE_UUID    0xFFB0
#define BLE_GESTURE_UUID    0xFFB1
#define BLE_BATTERY_UUID    0xFFB2
#define BLE_HEARTBEAT_UUID  0xFFB3
#define BLE_SCAN_TIMEOUT    5000  // ms
#define BLE_HEARTBEAT_MS    2000  // ms
```

### 5.2 戒指端配置

```cpp
// === 引脚定义 ===
#define PIN_I2C_SDA     2      // P0.02
#define PIN_I2C_SCL     3      // P0.03
#define PIN_LED         28     // P0.28
#define PIN_BAT_SENSE   4      // P0.04

// === IMU 配置 ===
#define IMU_SAMPLE_RATE   100   // Hz
#define IMU_I2C_ADDR      0x69

// === 手势识别配置 ===
#define GESTURE_WINDOW_MS    1500  // 录制窗口
#define GESTURE_TRIGGER_TH   2.0   // 触发阈值 (m/s²)
#define GESTURE_TEMPLATE_NUM 5     // 模板数量

// === BLE 配置 ===
#define BLE_SERVICE_UUID    0xFFB0
#define BLE_GESTURE_UUID    0xFFB1
#define BLE_BATTERY_UUID    0xFFB2
#define BLE_HEARTBEAT_UUID  0xFFB3
#define BLE_ADV_INTERVAL    100   // ms
#define BLE_CONN_INTERVAL   30    // ms
#define BLE_TIMEOUT         5000  // ms
```

---

## 6. 烧录说明

### 6.1 耳机端烧录

1. USB-C 连接 XIAO ESP32C3
2. 按住 BOOT 键，再按 RST 键，进入下载模式
3. Arduino IDE 选择开发板: `XIAO ESP32C3`
4. 上传固件
5. 将 microSD 卡格式化为 FAT32，放入 MP3 文件和 `bpm_list.csv`

### 6.2 戒指端烧录

1. USB-C 连接 XIAO nRF52840
2. 快速双击 RST 键，进入 bootloader 模式
3. Arduino IDE 选择开发板: `Seeed XIAO nRF52840`
4. 上传固件

---

## 7. 调试方法

### 7.1 串口日志

| 端 | 波特率 | 日志内容 |
|----|--------|---------|
| 耳机端 | 115200 | 步频、切歌、BLE 状态、按键事件 |
| 戒指端 | 115200 | 手势事件、DTW 距离、BLE 状态 |

### 7.2 LED 指示

| 状态 | 耳机端 LED | 戒指端 LED |
|------|-----------|-----------|
| 初始化中 | 慢闪 (1Hz) | 慢闪 (1Hz) |
| 正常运行 | 常亮 | 熄灭 |
| BLE 未连接 | 快闪 (4Hz) | 快闪 (4Hz) |
| BLE 已连接 | 常亮 | 常亮 |
| 充电中 | 脉冲呼吸 | — |
| 充满 | 常亮 | — |
| 电量低 (<15%) | 双闪 | 双闪 |

---

## 8. 工具脚本

### 8.1 BPM 标注工具（tools/bpm_tagger.py）

```bash
python tools/bpm_tagger.py /path/to/music/
# 自动分析 MP3 文件 BPM，生成 bpm_list.csv
```

### 8.2 手势模板采集（tools/gesture_trainer.py）

```bash
# 需配合戒指端串口使用
python tools/gesture_trainer.py --port COM3
# 交互式录制 5 类手势模板，保存为 templates.h
```

---

## 9. 版本历史

| 版本 | 日期 | 变更说明 |
|------|------|---------|
| v1.0 | 2026/07/15 | 初始版本，代码骨架 + 文档 |
