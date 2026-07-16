/*
 * BeatSync 手势识别模块实现（DTW）
 */
#include "gesture.h"
#include "config.h"
#include <Wire.h>
#include <ICM42688.h>

ICM42688 IMU(Wire, IMU_I2C_ADDR);

void GestureRecognizer::begin() {
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  IMU.begin();
  IMU.setAccelODR(ICM42688::odr100hz);
  IMU.setAccelFS(ICM42688::dpm4g);
  IMU.setGyroODR(ICM42688::odr100hz);
  IMU.setGyroFS(ICM42688::dps2000);

  bufferLen = 0;
  recording = false;
  recordStart = 0;
  templateCount = 0;
  newGesture = false;
  lastGesture = 0;
  lastConfidence = 0;

  Serial.println("[GESTURE] Ready");
}

void GestureRecognizer::readIMU(IMUData& d) {
  IMU.getAGT();
  d.ax = IMU.accX();
  d.ay = IMU.accY();
  d.az = IMU.accZ();
  d.gx = IMU.gyrX();
  d.gy = IMU.gyrY();
  d.gz = IMU.gyrZ();
}

float GestureRecognizer::calcEnergy(IMUData& d) {
  // 加速度能量（去除重力后的变化量）
  static float prevAx = 0, prevAy = 0, prevAz = 0;
  float dx = d.ax - prevAx;
  float dy = d.ay - prevAy;
  float dz = d.az - prevAz;
  prevAx = d.ax; prevAy = d.ay; prevAz = d.az;
  return sqrt(dx*dx + dy*dy + dz*dz);
}

float GestureRecognizer::dtwDistance(IMUData* a, int lenA, IMUData* b, int lenB) {
  // 简化 DTW：使用加速度模长序列
  // 完整实现应考虑 6 轴
  if (lenA == 0 || lenB == 0) return 1e9;

  // 降采样到固定长度（减少计算量）
  int targetLen = 30;
  float seqA[30], seqB[30];

  for (int i = 0; i < targetLen; i++) {
    int idxA = (int)((float)i / targetLen * lenA);
    int idxB = (int)((float)i / targetLen * lenB);
    seqA[i] = sqrt(a[idxA].ax*a[idxA].ax + a[idxA].ay*a[idxA].ay + a[idxA].az*a[idxA].az);
    seqB[i] = sqrt(b[idxB].ax*b[idxB].ax + b[idxB].ay*b[idxB].ay + b[idxB].az*b[idxB].az);
  }

  // DTW 距离矩阵（使用环形缓冲节省内存）
  float prev[31], curr[31];
  for (int j = 0; j <= targetLen; j++) prev[j] = 1e9;
  prev[0] = 0;

  for (int i = 1; i <= targetLen; i++) {
    curr[0] = 1e9;
    for (int j = 1; j <= targetLen; j++) {
      float cost = abs(seqA[i-1] - seqB[j-1]);
      float minPrev = prev[j-1];
      if (prev[j] < minPrev) minPrev = prev[j];
      if (curr[j-1] < minPrev) minPrev = curr[j-1];
      curr[j] = cost + minPrev;
    }
    for (int j = 0; j <= targetLen; j++) prev[j] = curr[j];
  }

  return prev[targetLen] / targetLen; // 归一化
}

void GestureRecognizer::update() {
  unsigned long now = millis();

  // 100Hz 采样
  static unsigned long lastSample = 0;
  if (now - lastSample < 10) return;
  lastSample = now;

  IMUData d;
  readIMU(d);

  if (!recording) {
    // 检测触发
    float energy = calcEnergy(d);
    if (energy > GESTURE_TRIGGER_TH) {
      recording = true;
      recordStart = now;
      bufferLen = 0;
      Serial.println("[GESTURE] Recording started");
    }
  }

  if (recording) {
    if (bufferLen < GESTURE_MAX_SAMPLES) {
      buffer[bufferLen++] = d;
    }

    // 录制窗口结束
    if (now - recordStart >= GESTURE_WINDOW_MS) {
      recording = false;

      // DTW 匹配
      float minDist = 1e9;
      int bestMatch = 0;

      for (int t = 0; t < templateCount; t++) {
        float dist = dtwDistance(buffer, bufferLen,
                                  templates[t].samples, templates[t].length);
        if (dist < minDist) {
          minDist = dist;
          bestMatch = t;
        }
      }

      if (templateCount > 0 && minDist < 100.0) { // 阈值可调
        lastGesture = templates[bestMatch].id;
        // 置信度：距离越小置信度越高
        lastConfidence = (uint8_t)max(0.0, min(100.0, 100.0 - minDist));
        newGesture = true;
        Serial.printf("[GESTURE] ID=0x%02X conf=%d dist=%.2f\n",
                       lastGesture, lastConfidence, minDist);
      } else {
        Serial.println("[GESTURE] No match");
      }
    }
  }
}

uint8_t GestureRecognizer::getGesture() {
  newGesture = false;
  return lastGesture;
}

uint8_t GestureRecognizer::getConfidence() {
  return lastConfidence;
}

bool GestureRecognizer::hasNewGesture() {
  return newGesture;
}

void GestureRecognizer::loadTemplates() {
  // TODO: 从 Flash 加载预存模板
  // 原型阶段可通过串口录制后硬编码
  Serial.println("[GESTURE] Templates loaded (placeholder)");
  templateCount = 0;
}

void GestureRecognizer::recordTemplate(uint8_t id) {
  if (templateCount < GESTURE_TEMPLATE_NUM) {
    templates[templateCount].id = id;
    for (int i = 0; i < bufferLen && i < GESTURE_MAX_SAMPLES; i++) {
      templates[templateCount].samples[i] = buffer[i];
    }
    templates[templateCount].length = bufferLen;
    templateCount++;
    Serial.printf("[GESTURE] Template %d recorded (id=0x%02X, len=%d)\n",
                   templateCount - 1, id, bufferLen);
  }
}
