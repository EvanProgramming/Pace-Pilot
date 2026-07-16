/*
 * BeatSync 步频检测模块实现
 */
#include "cadence.h"
#include "config.h"
#include <Wire.h>
#include <ICM42688.h>

ICM42688 IMU(Wire, IMU_I2C_ADDR);

void CadenceDetector::begin() {
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  IMU.begin();
  IMU.setAccelODR(ICM42688::odr100hz);
  IMU.setAccelFS(ICM42688::dpm4g);

  bufferIndex = 0;
  currentBPM = 0;
  running = false;
  lastUpdate = 0;
  lastSample = 0;

  for (int i = 0; i < WINDOW_SIZE; i++) {
    accelBuffer[i] = 0;
  }
}

float CadenceDetector::getAccelZ() {
  IMU.getAGT();
  return IMU.accZ();
}

void CadenceDetector::readIMU() {
  IMU.getAGT();
  float z = IMU.accZ();

  // 去除重力分量 (简单高通)
  static float prevZ = 0;
  float filtered = z - prevZ;
  prevZ = z;

  accelBuffer[bufferIndex] = filtered;
  bufferIndex = (bufferIndex + 1) % WINDOW_SIZE;
}

void CadenceDetector::bandpassFilter() {
  // 简易滑动平均（低通部分）
  // 完整实现应使用 Butterworth 带通
  float sum = 0;
  for (int i = 0; i < 10; i++) {
    int idx = (bufferIndex - i + WINDOW_SIZE) % WINDOW_SIZE;
    sum += accelBuffer[idx];
  }
  // 存储滤波结果（简化处理）
}

int CadenceDetector::detectPeaks() {
  int peakCount = 0;
  float threshold = CADENCE_RUN_THRESH;

  for (int i = 2; i < WINDOW_SIZE - 2; i++) {
    int idx = (bufferIndex + i) % WINDOW_SIZE;
    float v = accelBuffer[idx];

    // 简单峰值检测
    if (v > threshold &&
        v > accelBuffer[(idx - 1 + WINDOW_SIZE) % WINDOW_SIZE] &&
        v > accelBuffer[(idx + 1) % WINDOW_SIZE]) {
      peakCount++;
    }
  }
  return peakCount;
}

float CadenceDetector::calculateBPM(int peakCount) {
  // 窗口 2 秒，峰值数 = 步数
  // BPM = 步数 / 2秒 * 60 = 步数 * 30
  return peakCount * 30.0;
}

void CadenceDetector::update() {
  unsigned long now = millis();

  // 100Hz 采样
  if (now - lastSample >= 10) {
    readIMU();
    lastSample = now;
  }

  // 1Hz 更新步频
  if (now - lastUpdate >= UPDATE_INTERVAL) {
    bandpassFilter();
    int peaks = detectPeaks();
    currentBPM = calculateBPM(peaks);
    running = (currentBPM > 60 && currentBPM < 240);
    lastUpdate = now;

    Serial.printf("[CADENCE] BPM=%.1f running=%d\n", currentBPM, running);
  }
}

float CadenceDetector::getBPM() {
  return currentBPM;
}

bool CadenceDetector::isRunning() {
  return running;
}
