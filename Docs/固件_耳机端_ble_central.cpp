/*
 * BeatSync BLE 主机模块实现（耳机端）
 */
#include "ble_central.h"
#include "config.h"
#include <BLEDevice.h>

static BLEAddress* targetDeviceAddr = nullptr;
static BLERemoteCharacteristic* gestureChar = nullptr;
static BLERemoteCharacteristic* batteryChar = nullptr;
static bool doConnect = false;
static bool bleConnected = false;

static void notifyCallback(BLERemoteCharacteristic* ch, uint8_t* data, size_t len, bool isNotify) {
  if (len >= 1 && ch->getUUID().equals(BLEUUID(BLE_GESTURE_UUID))) {
    uint8_t gestureId = data[0];
    // 通过全局函数传递
    extern void handleGestureData(uint8_t);
    handleGestureData(gestureId);
  }
}

// 全局回调桥接
static GestureCallback gGestureCallback = nullptr;

void handleGestureData(uint8_t gestureId) {
  if (gGestureCallback) {
    gGestureCallback(gestureId);
  }
}

void BLECentral::begin() {
  BLEDevice::init("BeatSync-Earphone");
  connected = false;
  gestureCallback = nullptr;
  lastDataTime = 0;
}

void BLECentral::onGesture(GestureCallback cb) {
  gestureCallback = cb;
  gGestureCallback = cb;
}

void BLECentral::scanAndConnect() {
  Serial.println("[BLE] Scanning...");
  BLEScan* scan = BLEDevice::getScan();
  scan->setActiveScan(true);

  BLEScanResults results = scan->start(BLE_SCAN_TIMEOUT / 1000);
  for (int i = 0; i < results.getCount(); i++) {
    BLEAdvertisedDevice dev = results.getDevice(i);
    if (dev.getName() == "BeatSync-Ring") {
      Serial.println("[BLE] Found BeatSync-Ring!");
      targetDeviceAddr = new BLEAddress(dev.getAddress());
      doConnect = true;
      break;
    }
  }
}

bool BLECentral::isConnected() {
  return bleConnected;
}

void BLECentral::update() {
  if (doConnect) {
    doConnect = false;
    BLEClient* client = BLEDevice::createClient();
    if (client->connect(*targetDeviceAddr)) {
      Serial.println("[BLE] Connected to Ring!");

      BLERemoteService* service = client->getService(BLEUUID(BLE_SERVICE_UUID));
      if (service) {
        gestureChar = service->getCharacteristic(BLEUUID(BLE_GESTURE_UUID));
        batteryChar = service->getCharacteristic(BLEUUID(BLE_BATTERY_UUID));

        if (gestureChar) {
          gestureChar->registerForNotify(notifyCallback);
          Serial.println("[BLE] Subscribed to gesture notifications");
        }
      }
      bleConnected = true;
    } else {
      Serial.println("[BLE] Connection failed");
    }
  }

  // 超时检测
  if (bleConnected && millis() - lastDataTime > BLE_TIMEOUT) {
    // TODO: 重连逻辑
  }
}

void BLECentral::sendHeartbeat() {
  if (bleConnected && batteryChar) {
    // 写入心跳值
    uint8_t heartbeat = 0xAA;
    // 通过 heartbeat characteristic 写入
    // TODO: 实现 heartbeat write
    lastDataTime = millis();
  }
}
