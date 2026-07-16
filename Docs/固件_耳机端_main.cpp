/*
 * BeatSync 耳机端主程序
 * Platform: Seeed XIAO ESP32C3
 */

#include <Arduino.h>
#include <Wire.h>
#include <BLEDevice.h>
#include "config.h"
#include "cadence.h"
#include "player.h"
#include "ble_central.h"
#include "bpm_matcher.h"
#include "buttons.h"

// 全局对象
CadenceDetector cadence;
MusicPlayer player;
BLECentral bleCentral;
BPMMatcher bpmMatcher;
ButtonHandler buttons;

// 状态机
enum SystemState {
  STATE_INIT,
  STATE_SCANNING,
  STATE_RUNNING,
  STATE_LOW_BATTERY,
};

SystemState state = STATE_INIT;
bool bpmMatchEnabled = true;
unsigned long lastHeartbeat = 0;
unsigned long lastBatteryUpdate = 0;

// 手势回调
void onGestureReceived(uint8_t gestureId) {
  Serial.printf("[GESTURE] Received: 0x%02X\n", gestureId);
  switch (gestureId) {
    case 0x01: // 单击 - 播放/暂停
      player.togglePlayPause();
      break;
    case 0x02: // 双击 - 下一首
      player.next();
      break;
    case 0x03: // 上滑 - 音量+
      player.volumeUp();
      break;
    case 0x04: // 下滑 - 音量-
      player.volumeDown();
      break;
    case 0x05: // 旋转 - 步频匹配 开/关
      bpmMatchEnabled = !bpmMatchEnabled;
      Serial.printf("[BPM] Match %s\n", bpmMatchEnabled ? "ON" : "OFF");
      break;
    default:
      Serial.printf("[GESTURE] Unknown: 0x%02X\n", gestureId);
  }
}

// 按键回调
void onButtonEvent(uint8_t btnId, bool isLongPress) {
  switch (btnId) {
    case BTN_MODE:
      if (!isLongPress) {
        bpmMatchEnabled = !bpmMatchEnabled;
        Serial.printf("[BPM] Match %s\n", bpmMatchEnabled ? "ON" : "OFF");
      }
      break;
    case BTN_VOL_UP:
      if (isLongPress) {
        for (int i = 0; i < 5; i++) player.volumeUp();
      } else {
        player.volumeUp();
      }
      break;
    case BTN_VOL_DN:
      if (isLongPress) {
        for (int i = 0; i < 5; i++) player.volumeDown();
      } else {
        player.volumeDown();
      }
      break;
  }
}

// 电池检测
uint8_t readBatteryLevel() {
  int adc = analogRead(PIN_BAT_SENSE);
  float voltage = (adc / (float)BAT_ADC_MAX) * BAT_VREF * BAT_DIVIDER;
  float pct = (voltage - BAT_EMPTY) / (BAT_FULL - BAT_EMPTY) * 100.0;
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;
  return (uint8_t)pct;
}

void setup() {
  Serial.begin(115200);
  Serial.println("\n=== BeatSync Earphone Boot ===");

  // 初始化引脚
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_BTN_MODE, INPUT_PULLUP);
  pinMode(PIN_BTN_VOL_UP, INPUT_PULLUP);
  pinMode(PIN_BTN_VOL_DN, INPUT_PULLUP);
  pinMode(PIN_CHG_STATUS, INPUT_PULLUP);
  analogReadResolution(12);

  // 初始化 IMU + 步频检测
  cadence.begin();
  Serial.println("[INIT] Cadence detector ready");

  // 初始化播放器
  player.begin();
  Serial.println("[INIT] Music player ready");

  // 初始化 BPM 匹配
  bpmMatcher.begin();
  Serial.println("[INIT] BPM matcher ready");

  // 初始化按键
  buttons.begin(onButtonEvent);
  Serial.println("[INIT] Buttons ready");

  // 初始化 BLE
  bleCentral.begin();
  bleCentral.onGesture(onGestureReceived);
  Serial.println("[INIT] BLE central ready");

  state = STATE_SCANNING;
  Serial.println("[STATE] Scanning for ring...");
}

void loop() {
  // 更步频检测
  cadence.update();
  float bpm = cadence.getBPM();

  // 更新播放器
  player.update();

  // 更新按键
  buttons.update();

  // BLE 连接管理
  bleCentral.update();

  // 心跳
  if (bleCentral.isConnected() && millis() - lastHeartbeat > BLE_HEARTBEAT_MS) {
    bleCentral.sendHeartbeat();
    lastHeartbeat = millis();
  }

  // 步频匹配切歌
  if (bpmMatchEnabled && bleCentral.isConnected()) {
    bpmMatcher.update(bpm);
    if (bpmMatcher.shouldSwitch()) {
      uint16_t track = bpmMatcher.getMatchedTrack();
      Serial.printf("[BPM] Switching to track %d (cadence=%.1f)\n", track, bpm);
      player.play(track);
    }
  }

  // 电池检测（每 30 秒）
  if (millis() - lastBatteryUpdate > 30000) {
    uint8_t batLevel = readBatteryLevel();
    Serial.printf("[BAT] Level: %d%%\n", batLevel);
    if (batLevel < BAT_LOW_THRESH) {
      state = STATE_LOW_BATTERY;
    }
    lastBatteryUpdate = millis();
  }

  // 状态 LED
  switch (state) {
    case STATE_INIT:
      // LED 慢闪
      digitalWrite(PIN_LED, (millis() / 500) % 2);
      break;
    case STATE_SCANNING:
      // LED 快闪
      digitalWrite(PIN_LED, (millis() / 125) % 2);
      if (bleCentral.isConnected()) {
        state = STATE_RUNNING;
        digitalWrite(PIN_LED, HIGH);
        Serial.println("[STATE] Running");
      }
      break;
    case STATE_RUNNING:
      // LED 常亮
      digitalWrite(PIN_LED, HIGH);
      break;
    case STATE_LOW_BATTERY:
      // LED 双闪
      {
        int phase = (millis() % 1000);
        digitalWrite(PIN_LED, (phase < 100 || (phase > 200 && phase < 300)) ? HIGH : LOW);
      }
      break;
  }

  delay(10);
}
