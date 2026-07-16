/*
 * BeatSync 戒指端固件 - 全局配置
 * Platform: Seeed XIAO nRF52840
 */

#ifndef CONFIG_H
#define CONFIG_H

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
#define GESTURE_SAMPLE_RATE  100   // Hz
#define GESTURE_MAX_SAMPLES  150   // 1.5s × 100Hz

// 手势 ID
#define GESTURE_SINGLE_TAP  0x01
#define GESTURE_DOUBLE_TAP  0x02
#define GESTURE_SWIPE_UP    0x03
#define GESTURE_SWIPE_DOWN  0x04
#define GESTURE_ROTATE      0x05

// === BLE 配置 ===
#define BLE_DEVICE_NAME      "BeatSync-Ring"
#define BLE_SERVICE_UUID     0xFFB0
#define BLE_GESTURE_UUID     0xFFB1
#define BLE_BATTERY_UUID     0xFFB2
#define BLE_HEARTBEAT_UUID   0xFFB3
#define BLE_ADV_INTERVAL     100   // ms
#define BLE_CONN_INTERVAL    30    // ms
#define BLE_TIMEOUT          5000  // ms

// === 电池配置 ===
#define BAT_ADC_MAX     4095    // 12-bit ADC
#define BAT_VREF        3.3     // V
#define BAT_DIVIDER     2.0     // 1:2 分压
#define BAT_FULL        4.2     // V
#define BAT_EMPTY       3.3     // V
#define BAT_LOW_THRESH  15      // %

#endif // CONFIG_H
