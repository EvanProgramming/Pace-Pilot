/*
 * BeatSync 手势识别模块（DTW）
 */
#ifndef GESTURE_H
#define GESTURE_H

#include <Arduino.h>

// 6 轴数据点
struct IMUData {
  float ax, ay, az;  // 加速度
  float gx, gy, gz;  // 陀螺仪
};

// 手势模板
struct GestureTemplate {
  uint8_t id;
  IMUData samples[GESTURE_MAX_SAMPLES];
  int length;
};

class GestureRecognizer {
public:
  void begin();
  void update();
  uint8_t getGesture();     // 获取识别结果
  uint8_t getConfidence();  // 获取置信度
  bool hasNewGesture();
  void loadTemplates();
  void recordTemplate(uint8_t id);

private:
  IMUData buffer[GESTURE_MAX_SAMPLES];
  int bufferLen;
  bool recording;
  unsigned long recordStart;

  GestureTemplate templates[GESTURE_TEMPLATE_NUM];
  int templateCount;

  uint8_t lastGesture;
  uint8_t lastConfidence;
  bool newGesture;

  float calcEnergy(IMUData& d);
  float dtwDistance(IMUData* a, int lenA, IMUData* b, int lenB);
  void readIMU(IMUData& d);
};

#endif
