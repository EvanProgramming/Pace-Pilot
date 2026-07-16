/*
 * BeatSync BLE 从机模块（戒指端）
 */
#ifndef BLE_PERIPHERAL_H
#define BLE_PERIPHERAL_H

#include <Arduino.h>

class BLEPeripheral {
public:
  void begin();
  void update();
  void notifyGesture(uint8_t gestureId, uint8_t confidence);
  void notifyBattery(uint8_t level);
  bool isConnected();
  bool heartbeatReceived();

private:
  bool connected;
  bool heartbeat;
  unsigned long lastHeartbeat;
};

#endif
