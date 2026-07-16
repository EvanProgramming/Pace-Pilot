/*
 * BeatSync 按键处理模块
 */
#ifndef BUTTONS_H
#define BUTTONS_H

#include <Arduino.h>
#include "config.h"

enum ButtonId {
  BTN_MODE = 0,
  BTN_VOL_UP = 1,
  BTN_VOL_DN = 2,
};

typedef void (*ButtonCallback)(uint8_t btnId, bool isLongPress);

class ButtonHandler {
public:
  void begin(ButtonCallback cb);
  void update();

private:
  ButtonCallback callback;
  bool prevState[3];
  bool pressed[3];
  unsigned long pressTime[3];
  bool longPressFired[3];

  static const uint8_t pinMap[3];
};

#endif
