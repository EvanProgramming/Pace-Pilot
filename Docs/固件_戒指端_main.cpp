/*
 * BeatSync 戒指端主程序
 * Platform: Seeed XIAO nRF52840
 */

#include <Arduino.h>
#include "config.h"
#include "gesture.h"
#include "ble_peripheral.h"

GestureRecognizer gesture;
BLEPeripheral blePeripheral;

unsigned long lastBatteryUpdate = 0;

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
  Serial.println("\n=== BeatSync Ring Boot ===");

  pinMode(PIN_LED, OUTPUT);
  analogReadResolution(12);

  // 初始化手势识别
  gesture.begin();
  gesture.loadTemplates();

  // 初始化 BLE
  blePeripheral.begin();

  Serial.println("[INIT] Ring ready");
}

void loop() {
  // 更新手势识别
  gesture.update();

  // 检测到手势 → BLE 通知
  if (gesture.hasNewGesture()) {
    uint8_t g = gesture.getGesture();
    uint8_t conf = gesture.getConfidence();
    blePeripheral.notifyGesture(g, conf);
  }

  // BLE 更新
  blePeripheral.update();

  // 电量检测（每 60 秒）
  if (millis() - lastBatteryUpdate > 60000) {
    uint8_t batLevel = readBatteryLevel();
    Serial.printf("[BAT] Level: %d%%\n", batLevel);
    blePeripheral.notifyBattery(batLevel);
    lastBatteryUpdate = millis();
  }

  // LED 状态指示
  if (blePeripheral.isConnected()) {
    digitalWrite(PIN_LED, HIGH); // 常亮
  } else {
    digitalWrite(PIN_LED, (millis() / 125) % 2); // 快闪
  }

  delay(5);
}
