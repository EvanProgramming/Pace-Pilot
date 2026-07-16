/*
 * BeatSync BLE 主机模块（耳机端）
 */
#ifndef BLE_CENTRAL_H
#define BLE_CENTRAL_H

#include <Arduino.h>

typedef void (*GestureCallback)(uint8_t gestureId);

class BLECentral {
public:
  void begin();
  void update();
  void scanAndConnect();
  bool isConnected();
  void onGesture(GestureCallback cb);
  void sendHeartbeat();

private:
  bool connected;
  GestureCallback gestureCallback;
  unsigned long lastDataTime;
};

#endif
