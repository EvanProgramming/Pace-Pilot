/*
 * BeatSync 按键处理模块实现
 */
#include "buttons.h"
#include "config.h"

const uint8_t ButtonHandler::pinMap[3] = {PIN_BTN_MODE, PIN_BTN_VOL_UP, PIN_BTN_VOL_DN};

void ButtonHandler::begin(ButtonCallback cb) {
  callback = cb;
  for (int i = 0; i < 3; i++) {
    pinMode(pinMap[i], INPUT_PULLUP);
    prevState[i] = HIGH;
    pressed[i] = false;
    pressTime[i] = 0;
    longPressFired[i] = false;
  }
}

void ButtonHandler::update() {
  unsigned long now = millis();

  for (int i = 0; i < 3; i++) {
    bool curState = digitalRead(pinMap[i]);

    // 下降沿 - 按下
    if (prevState[i] == HIGH && curState == LOW) {
      if (now - pressTime[i] > BTN_DEBOUNCE_MS) {
        pressed[i] = true;
        pressTime[i] = now;
        longPressFired[i] = false;
      }
    }

    // 上升沿 - 释放
    if (prevState[i] == LOW && curState == HIGH) {
      if (pressed[i] && !longPressFired[i]) {
        // 短按事件
        if (callback) {
          callback(i, false);
        }
      }
      pressed[i] = false;
    }

    // 长按检测
    if (pressed[i] && !longPressFired[i]) {
      if (now - pressTime[i] >= BTN_LONG_PRESS) {
        longPressFired[i] = true;
        if (callback) {
          callback(i, true);
        }
      }
    }

    prevState[i] = curState;
  }
}
