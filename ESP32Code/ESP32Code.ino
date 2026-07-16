#include <Wire.h>

// ============================================================
// PacePilot ESP32 Z-Axis Motion Controller
//
// MPU6050：
// VCC -> ESP32 3V3
// GND -> ESP32 GND
// SDA -> ESP32 GPIO21
// SCL -> ESP32 GPIO22
//
// 主板通信：
// ESP32 GPIO17 TX -> Arduino D11
// ESP32 GND       -> Arduino GND
//
// 数据格式：
// P,BPM,SPM,INTENSITY,STEP,ZONE
// ============================================================


// ============================================================
// 引脚
// ============================================================

constexpr int I2C_SDA_PIN = 21;
constexpr int I2C_SCL_PIN = 22;

constexpr int UART_RX_PIN = 16;  // 不连接
constexpr int UART_TX_PIN = 17;  // 接 Arduino D11


// ============================================================
// MPU6050
// ============================================================

constexpr uint8_t MPU6050_ADDRESS = 0x68;

constexpr uint8_t REG_PWR_MGMT_1   = 0x6B;
constexpr uint8_t REG_SMPLRT_DIV   = 0x19;
constexpr uint8_t REG_CONFIG       = 0x1A;
constexpr uint8_t REG_ACCEL_CONFIG = 0x1C;
constexpr uint8_t REG_ACCEL_XOUT_H = 0x3B;


// ============================================================
// 采样与发送
// ============================================================

constexpr unsigned long SAMPLE_INTERVAL_MS = 10;   // 100 Hz
constexpr unsigned long SEND_INTERVAL_MS = 200;    // 5 Hz


// ============================================================
// 步伐时间范围
// ============================================================

// 最快约 240 SPM
constexpr unsigned long MIN_STEP_INTERVAL_MS = 250;

// 最慢约 40 SPM
constexpr unsigned long MAX_STEP_INTERVAL_MS = 1500;


// ============================================================
// 停止与速度保持
// ============================================================

// 漏检后前 3 秒保持最后速度
constexpr unsigned long FULL_HOLD_TIME_MS = 3000;

// 超过 8 秒没有新步伐才真正归零
constexpr unsigned long STOP_TIMEOUT_MS = 8000;


// ============================================================
// Z 轴灵敏度
//
// 数值越小越灵敏，越大越不灵敏。
//
// 推荐：
// 0.05 = 很灵敏
// 0.08 = 中高灵敏度
// 0.12 = 中等灵敏度
// 0.18 = 较低灵敏度
//
// 建议先使用 0.10。
// ============================================================

constexpr float Z_PEAK_THRESHOLD = 0.10f;


// 峰值必须比两侧高出的最小值
// 越大越不容易把小抖动识别成步伐
constexpr float MIN_PEAK_PROMINENCE = 0.015f;


// Z 轴信号平滑比例
//
// 越大越平滑，但响应越慢。
// 推荐范围 0.35～0.70。
constexpr float Z_FILTER_OLD_WEIGHT = 0.45f;


// ============================================================
// 步间隔历史
// ============================================================

constexpr uint8_t INTERVAL_HISTORY_SIZE = 8;

unsigned long stepIntervals[INTERVAL_HISTORY_SIZE];

uint8_t intervalCount = 0;
uint8_t intervalIndex = 0;


// ============================================================
// 速度区间
// ============================================================

enum PaceZone {
  ZONE_IDLE = 0,
  ZONE_WARMUP = 1,
  ZONE_JOG = 2,
  ZONE_RUN = 3,
  ZONE_SPRINT = 4
};


// ============================================================
// 时间变量
// ============================================================

unsigned long lastSampleTime = 0;
unsigned long lastSendTime = 0;
unsigned long lastStepTime = 0;


// ============================================================
// 运动变量
// ============================================================

uint32_t stepCounter = 0;

// 当前原始 Z 轴加速度
float rawZ = 0.0f;

// Z 轴静止/重力基准
// 安装方向不同，可能逐渐稳定在 +1 或 -1 附近
float zBaseline = 0.0f;

// 相对基准的 Z 轴运动量
float zMotion = 0.0f;

// 平滑后的 Z 轴运动量
float filteredZMotion = 0.0f;

// 三点局部峰值
float zSignalMinus2 = 0.0f;
float zSignalMinus1 = 0.0f;
float zSignalCurrent = 0.0f;


// ============================================================
// 步频变量
// ============================================================

float rawSPM = 0.0f;
float stableSPM = 0.0f;

int outputBPM = 90;
int intensity = 0;

PaceZone currentZone = ZONE_IDLE;


// ============================================================
// MPU6050 写寄存器
// ============================================================

bool writeMPURegister(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(MPU6050_ADDRESS);
  Wire.write(reg);
  Wire.write(value);

  return Wire.endTransmission() == 0;
}


// ============================================================
// 初始化 MPU6050
// ============================================================

bool initializeMPU6050() {
  // 唤醒
  if (!writeMPURegister(REG_PWR_MGMT_1, 0x00)) {
    return false;
  }

  delay(100);

  // 100 Hz：
  // 1 kHz / (1 + 9)
  if (!writeMPURegister(REG_SMPLRT_DIV, 9)) {
    return false;
  }

  // DLPF = 2
  if (!writeMPURegister(REG_CONFIG, 0x02)) {
    return false;
  }

  // 加速度计 ±2g，最高灵敏度
  if (!writeMPURegister(REG_ACCEL_CONFIG, 0x00)) {
    return false;
  }

  return true;
}


// ============================================================
// 读取三轴加速度
// ============================================================

bool readAcceleration(float &ax, float &ay, float &az) {
  Wire.beginTransmission(MPU6050_ADDRESS);
  Wire.write(REG_ACCEL_XOUT_H);

  if (Wire.endTransmission(false) != 0) {
    return false;
  }

  int received = Wire.requestFrom(
    static_cast<int>(MPU6050_ADDRESS),
    6,
    true
  );

  if (received != 6) {
    return false;
  }

  int16_t rawAX = static_cast<int16_t>(
    (Wire.read() << 8) | Wire.read()
  );

  int16_t rawAY = static_cast<int16_t>(
    (Wire.read() << 8) | Wire.read()
  );

  int16_t rawAZ = static_cast<int16_t>(
    (Wire.read() << 8) | Wire.read()
  );

  ax = rawAX / 16384.0f;
  ay = rawAY / 16384.0f;
  az = rawAZ / 16384.0f;

  return true;
}


// ============================================================
// 初始校准 Z 轴基准
//
// 开机时保持 MPU6050 静止约 1.5 秒。
// ============================================================

bool calibrateZBaseline() {
  constexpr int CALIBRATION_SAMPLES = 150;

  float totalZ = 0.0f;
  int validSamples = 0;

  Serial.println("正在校准 Z 轴，请保持传感器静止...");

  for (int i = 0; i < CALIBRATION_SAMPLES; i++) {
    float ax;
    float ay;
    float az;

    if (readAcceleration(ax, ay, az)) {
      totalZ += az;
      validSamples++;
    }

    delay(10);
  }

  if (validSamples < CALIBRATION_SAMPLES / 2) {
    return false;
  }

  zBaseline = totalZ / validSamples;

  Serial.print("Z 轴基准完成：");
  Serial.println(zBaseline, 4);

  return true;
}


// ============================================================
// 保存有效步间隔
// ============================================================

void addStepInterval(unsigned long interval) {
  stepIntervals[intervalIndex] = interval;

  intervalIndex++;

  if (intervalIndex >= INTERVAL_HISTORY_SIZE) {
    intervalIndex = 0;
  }

  if (intervalCount < INTERVAL_HISTORY_SIZE) {
    intervalCount++;
  }
}


// ============================================================
// 获取步间隔中位数
// ============================================================

unsigned long getMedianInterval() {
  if (intervalCount == 0) {
    return 0;
  }

  unsigned long temp[INTERVAL_HISTORY_SIZE];

  for (uint8_t i = 0; i < intervalCount; i++) {
    temp[i] = stepIntervals[i];
  }

  for (uint8_t i = 0; i < intervalCount - 1; i++) {
    for (uint8_t j = i + 1; j < intervalCount; j++) {
      if (temp[j] < temp[i]) {
        unsigned long swapValue = temp[i];
        temp[i] = temp[j];
        temp[j] = swapValue;
      }
    }
  }

  if (intervalCount % 2 == 1) {
    return temp[intervalCount / 2];
  }

  unsigned long lower = temp[intervalCount / 2 - 1];
  unsigned long upper = temp[intervalCount / 2];

  return (lower + upper) / 2;
}


// ============================================================
// 判断速度区间
// ============================================================

PaceZone determineZone(float spm) {
  if (spm < 70.0f) {
    return ZONE_IDLE;
  }

  if (spm < 120.0f) {
    return ZONE_WARMUP;
  }

  if (spm < 150.0f) {
    return ZONE_JOG;
  }

  if (spm < 175.0f) {
    return ZONE_RUN;
  }

  return ZONE_SPRINT;
}


// ============================================================
// 区间对应固定 BPM
// ============================================================

int getZoneBPM(PaceZone zone) {
  switch (zone) {
    case ZONE_IDLE:
      return 90;

    case ZONE_WARMUP:
      return 105;

    case ZONE_JOG:
      return 135;

    case ZONE_RUN:
      return 160;

    case ZONE_SPRINT:
      return 185;
  }

  return 90;
}


// ============================================================
// 计算运动强度
// ============================================================

int calculateIntensity(float spm) {
  if (spm <= 60.0f) {
    return 0;
  }

  if (spm >= 200.0f) {
    return 100;
  }

  float value =
    (spm - 60.0f) /
    140.0f *
    100.0f;

  return constrain(
    static_cast<int>(value),
    0,
    100
  );
}


// ============================================================
// 注册步伐
// ============================================================

void registerStep(unsigned long now) {
  // 第一脚只建立时间基准
  if (lastStepTime == 0) {
    lastStepTime = now;
    stepCounter++;
    return;
  }

  unsigned long interval = now - lastStepTime;

  // 太短：大概率是同一次上下振动里的第二个峰
  if (interval < MIN_STEP_INTERVAL_MS) {
    return;
  }

  // 太长：重新建立时间基准
  if (interval > MAX_STEP_INTERVAL_MS) {
    lastStepTime = now;
    stepCounter++;
    return;
  }

  addStepInterval(interval);

  unsigned long medianInterval = getMedianInterval();

  if (medianInterval > 0) {
    rawSPM =
      60000.0f /
      static_cast<float>(medianInterval);

    rawSPM = constrain(
      rawSPM,
      40.0f,
      240.0f
    );

    if (stableSPM <= 0.0f) {
      stableSPM = rawSPM;
    } else {
      // 88% 旧值 + 12% 新值
      // 减少速度数值不断变化
      stableSPM =
        0.88f * stableSPM +
        0.12f * rawSPM;
    }
  }

  lastStepTime = now;
  stepCounter++;
}


// ============================================================
// 处理 Z 轴运动
// ============================================================

void processMotion() {
  float ax;
  float ay;
  float az;

  if (!readAcceleration(ax, ay, az)) {
    return;
  }

  rawZ = az;

  // ==========================================================
  // 1. 缓慢追踪 Z 轴重力基准
  //
  // 跑步产生的是快速变化；
  // 传感器角度变化属于较慢变化。
  // ==========================================================

  zBaseline =
    0.998f * zBaseline +
    0.002f * rawZ;


  // ==========================================================
  // 2. 只获取 Z 轴相对基准的上下运动
  //
  // fabs() 表示向上或向下的明显冲击都可以形成峰值。
  // ==========================================================

  zMotion = fabs(rawZ - zBaseline);


  // ==========================================================
  // 3. 平滑 Z 轴运动
  // ==========================================================

  filteredZMotion =
    Z_FILTER_OLD_WEIGHT * filteredZMotion +
    (1.0f - Z_FILTER_OLD_WEIGHT) * zMotion;


  // ==========================================================
  // 4. 三点局部峰值检测
  // ==========================================================

  zSignalMinus2 = zSignalMinus1;
  zSignalMinus1 = zSignalCurrent;
  zSignalCurrent = filteredZMotion;

  float surroundingMaximum =
    max(zSignalMinus2, zSignalCurrent);

  bool isLocalPeak =
    zSignalMinus1 > zSignalMinus2 &&
    zSignalMinus1 > zSignalCurrent;

  bool isHighEnough =
    zSignalMinus1 >= Z_PEAK_THRESHOLD;

  bool isProminentEnough =
    (zSignalMinus1 - surroundingMaximum) >=
    MIN_PEAK_PROMINENCE;

  unsigned long now = millis();

  if (
    isLocalPeak &&
    isHighEnough &&
    isProminentEnough
  ) {
    registerStep(now);
  }


  // ==========================================================
  // 5. 漏检保持与停止检测
  // ==========================================================

  if (lastStepTime != 0) {
    unsigned long noStepTime =
      now - lastStepTime;

    // 0～3 秒完全保持最后速度
    if (noStepTime <= FULL_HOLD_TIME_MS) {
      // 不改变 stableSPM
    }

    // 3～8 秒逐渐衰减
    else if (noStepTime < STOP_TIMEOUT_MS) {
      stableSPM *= 0.9995f;
    }

    // 8 秒以上才真正归零
    else {
      stableSPM = 0.0f;
      rawSPM = 0.0f;

      intervalCount = 0;
      intervalIndex = 0;
    }
  }


  // ==========================================================
  // 6. 更新输出
  // ==========================================================

  currentZone = determineZone(stableSPM);
  outputBPM = getZoneBPM(currentZone);
  intensity = calculateIntensity(stableSPM);
}


// ============================================================
// 发送给 Arduino
// ============================================================

void sendDataToArduino() {
  int spm = static_cast<int>(
    stableSPM + 0.5f
  );

  Serial1.print("P,");
  Serial1.print(outputBPM);
  Serial1.print(",");
  Serial1.print(spm);
  Serial1.print(",");
  Serial1.print(intensity);
  Serial1.print(",");
  Serial1.print(stepCounter);
  Serial1.print(",");
  Serial1.println(
    static_cast<int>(currentZone)
  );

  // 调试输出
  Serial.print("SPM: ");
  Serial.print(spm);

  Serial.print(" | RawSPM: ");
  Serial.print(rawSPM, 1);

  Serial.print(" | Z: ");
  Serial.print(rawZ, 3);

  Serial.print(" | Baseline: ");
  Serial.print(zBaseline, 3);

  Serial.print(" | Z Motion: ");
  Serial.print(filteredZMotion, 3);

  Serial.print(" | Threshold: ");
  Serial.print(Z_PEAK_THRESHOLD, 3);

  Serial.print(" | Zone: ");
  Serial.print(
    static_cast<int>(currentZone)
  );

  Serial.print(" | Steps: ");
  Serial.println(stepCounter);
}


// ============================================================
// SETUP
// ============================================================

void setup() {
  Serial.begin(115200);

  Serial1.begin(
    9600,
    SERIAL_8N1,
    UART_RX_PIN,
    UART_TX_PIN
  );

  Wire.begin(
    I2C_SDA_PIN,
    I2C_SCL_PIN
  );

  Wire.setClock(400000);

  Serial.println();
  Serial.println("================================");
  Serial.println(" PacePilot Z-Axis Detector");
  Serial.println("================================");

  if (!initializeMPU6050()) {
    Serial.println("错误：无法连接 MPU6050。");

    while (true) {
      delay(1000);
    }
  }

  if (!calibrateZBaseline()) {
    Serial.println("错误：Z 轴校准失败。");

    while (true) {
      delay(1000);
    }
  }

  Serial.println("MPU6050 已就绪。");
  Serial.println("检测方式：Z 轴上下运动。");
  Serial.println();
}


// ============================================================
// LOOP
// ============================================================

void loop() {
  unsigned long now = millis();

  if (
    now - lastSampleTime >=
    SAMPLE_INTERVAL_MS
  ) {
    lastSampleTime = now;
    processMotion();
  }

  if (
    now - lastSendTime >=
    SEND_INTERVAL_MS
  ) {
    lastSendTime = now;
    sendDataToArduino();
  }
}