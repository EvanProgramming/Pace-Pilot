/*
 * BeatSync BLE 从机模块实现（戒指端）
 */
#include "ble_peripheral.h"
#include "config.h"
#include <bluefruit.h>

BLEService gestureService(BLEUUID(BLE_SERVICE_UUID));
BLECharacteristic gestureChar(BLEUUID(BLE_GESTURE_UUID), BLERead | BLENotify, 3);
BLECharacteristic batteryChar(BLEUUID(BLE_BATTERY_UUID), BLERead | BLENotify, 1);
BLECharacteristic heartbeatChar(BLEUUID(BLE_HEARTBEAT_UUID), BLEWrite, 1);

static bool bleConnected = false;
static bool hbReceived = false;

void connectCallback(uint16_t connHandle) {
  Serial.println("[BLE] Central connected!");
  bleConnected = true;
}

void disconnectCallback(uint16_t connHandle, uint8_t reason) {
  Serial.println("[BLE] Central disconnected");
  bleConnected = false;
  Bluefruit.Advertising.start();
}

void heartbeatWriteCallback(uint16_t connHandle, BLECharacteristic* chr, uint8_t* data, uint16_t len) {
  hbReceived = true;
}

void BLEPeripheral::begin() {
  Bluefruit.begin();
  Bluefruit.setTxPower(4);
  Bluefruit.setName(BLE_DEVICE_NAME);

  gestureService.begin();
  gestureChar.begin();
  batteryChar.begin();
  heartbeatChar.begin();

  gestureChar.write32(0);
  uint8_t batInit = 100;
  batteryChar.write(&batInit, 1);
  heartbeatChar.setWriteCallback(heartbeatWriteCallback);

  Bluefruit.Periph.setConnectCallback(connectCallback);
  Bluefruit.Periph.setDisconnectCallback(disconnectCallback);

  // 广播
  Bluefruit.Advertising.addService(gestureService);
  Bluefruit.Advertising.start();

  connected = false;
  heartbeat = false;
  lastHeartbeat = millis();

  Serial.println("[BLE] Peripheral advertising as BeatSync-Ring");
}

void BLEPeripheral::update() {
  connected = bleConnected;
  heartbeat = hbReceived;
  hbReceived = false;

  if (connected && heartbeat) {
    lastHeartbeat = millis();
  }
}

void BLEPeripheral::notifyGesture(uint8_t gestureId, uint8_t confidence) {
  if (bleConnected) {
    uint8_t data[3] = {gestureId, confidence, 0x00};
    gestureChar.notify(data, 3);
    Serial.printf("[BLE] Gesture notified: 0x%02X conf=%d\n", gestureId, confidence);
  }
}

void BLEPeripheral::notifyBattery(uint8_t level) {
  if (bleConnected) {
    batteryChar.notify(&level, 1);
  }
}

bool BLEPeripheral::isConnected() {
  return bleConnected;
}

bool BLEPeripheral::heartbeatReceived() {
  return heartbeat;
}
