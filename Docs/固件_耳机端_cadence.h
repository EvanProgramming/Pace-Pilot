/*
 * BeatSync 步频检测模块
 */
#ifndef CADENCE_H
#define CADENCE_H

#include <Arduino.h>

class CadenceDetector {
public:
  void begin();
  void update();
  float getBPM();
  bool isRunning();
  float getAccelZ();

private:
  static const int SAMPLE_RATE = 100;
  static const int WINDOW_SIZE = SAMPLE_RATE * 2; // 2 秒窗口
  static const int UPDATE_INTERVAL = 1000; // 1Hz 更新

  float accelBuffer[WINDOW_SIZE];
  int bufferIndex;
  unsigned long lastUpdate;
  unsigned long lastSample;

  float currentBPM;
  bool running;

  void readIMU();
  void bandpassFilter();
  int detectPeaks();
  float calculateBPM(int peakCount);
};

#endif
