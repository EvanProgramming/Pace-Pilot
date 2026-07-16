/*
 * BeatSync 耳机端固件 - 全局配置
 * Platform: Seeed XIAO ESP32C3
 */

#ifndef CONFIG_H
#define CONFIG_H

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
#define CADENCE_RUN_THRESH  1.5   // 跑步检测阈值 m/s²

// === DFPlayer 配置 ===
#define DFPLAYER_BAUD     9600
#define DFPLAYER_VOL_MAX  30
#define DFPLAYER_VOL_INIT 20

// === BPM 匹配配置 ===
#define BPM_MATCH_THRESHOLD  10     // BPM 差值阈值
#define BPM_MATCH_DURATION   10000  // 持续时间 ms
#define BPM_LIST_FILE        "/bpm_list.csv"

// === BLE 配置 ===
#define BLE_SERVICE_UUID    0xFFB0
#define BLE_GESTURE_UUID    0xFFB1
#define BLE_BATTERY_UUID    0xFFB2
#define BLE_HEARTBEAT_UUID  0xFFB3
#define BLE_SCAN_TIMEOUT    5000   // ms
#define BLE_HEARTBEAT_MS    2000   // ms
#define BLE_TIMEOUT         5000   // ms

// === 电池配置 ===
#define BAT_ADC_MAX     4095    // 12-bit ADC
#define BAT_VREF        3.3     // V
#define BAT_DIVIDER     2.0     // 1:2 分压
#define BAT_FULL        4.2     // V
#define BAT_EMPTY       3.3     // V
#define BAT_LOW_THRESH  15      // % 低电量阈值

// === 按键配置 ===
#define BTN_DEBOUNCE_MS  50     // 去抖动
#define BTN_LONG_PRESS   2000   // 长按时间

// === LED 模式 ===
enum LEDMode {
  LED_OFF,
  LED_ON,
  LED_BLINK_SLOW,    // 1Hz 初始化
  LED_BLINK_FAST,    // 4Hz BLE未连接
  LED_BREATH,        // 充电中
  LED_DOUBLE_BLINK,  // 低电量
};

#endif // CONFIG_H
