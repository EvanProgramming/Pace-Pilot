#include <SoftwareSerial.h>
#include <Adafruit_NeoPixel.h>

// ============================================================
// PacePilot - Rhythmic Running Music V2
//
// 接线：
// ESP32 GPIO17 TX -> Arduino D11
// ESP32 GND       -> Arduino GND
// Arduino D7      -> 扬声器 / 功放
// Arduino D5      -> WS2812 灯带 DIN
//
// ESP32 数据：
// P,BPM,SPM,INTENSITY,STEP,ZONE
// ============================================================


// ============================================================
// 硬件设置
// ============================================================

constexpr uint8_t ESP32_RX_PIN = 11;
constexpr uint8_t UNUSED_TX_PIN = 12;

constexpr uint8_t SPEAKER_PIN = 7;
constexpr uint8_t LED_PIN = 5;

// 修改成实际灯珠数量
constexpr uint16_t LED_COUNT = 12;

SoftwareSerial espSerial(
  ESP32_RX_PIN,
  UNUSED_TX_PIN
);

Adafruit_NeoPixel strip(
  LED_COUNT,
  LED_PIN,
  NEO_GRB + NEO_KHZ800
);


// ============================================================
// 音符
// ============================================================

constexpr uint16_t REST = 0;

// 鼓点与低音
constexpr uint16_t KICK_LOW   = 72;
constexpr uint16_t KICK_MAIN  = 88;
constexpr uint16_t KICK_HARD  = 105;
constexpr uint16_t BASS_C2    = 65;
constexpr uint16_t BASS_D2    = 73;
constexpr uint16_t BASS_E2    = 82;
constexpr uint16_t BASS_F2    = 87;
constexpr uint16_t BASS_G2    = 98;
constexpr uint16_t BASS_A2    = 110;

// 旋律
constexpr uint16_t NOTE_C4 = 262;
constexpr uint16_t NOTE_D4 = 294;
constexpr uint16_t NOTE_E4 = 330;
constexpr uint16_t NOTE_F4 = 349;
constexpr uint16_t NOTE_G4 = 392;
constexpr uint16_t NOTE_A4 = 440;
constexpr uint16_t NOTE_B4 = 494;

constexpr uint16_t NOTE_C5 = 523;
constexpr uint16_t NOTE_D5 = 587;
constexpr uint16_t NOTE_E5 = 659;
constexpr uint16_t NOTE_F5 = 698;
constexpr uint16_t NOTE_G5 = 784;
constexpr uint16_t NOTE_A5 = 880;


// ============================================================
// 事件类型
// ============================================================

enum SoundType {
  SOUND_REST,
  SOUND_KICK,
  SOUND_BASS,
  SOUND_MELODY
};


// 每一个事件占一个十六分音符格
struct SequenceStep {
  uint16_t frequency;
  uint8_t soundType;
  uint8_t gatePercent;
  uint8_t accent;
};


// ============================================================
// 跑步模式
// ============================================================

enum RunningMode {
  MODE_IDLE,
  MODE_WARMUP,
  MODE_JOG,
  MODE_RUN,
  MODE_SPRINT
};

RunningMode currentMode = MODE_IDLE;
RunningMode candidateMode = MODE_IDLE;


// ============================================================
// 模式切换控制
// ============================================================

// 新速度范围连续保持 5 秒
constexpr unsigned long MODE_CONFIRM_TIME_MS = 5000;

// 切换之后 5 秒内不允许再次切换
constexpr unsigned long MODE_COOLDOWN_MS = 5000;

unsigned long candidateModeStartTime = 0;
unsigned long lastModeChangeTime = 0;


// ============================================================
// 热身音乐
//
// 105 BPM
// 每四格一个主鼓点
// 鼓点轻，旋律较宽松
// ============================================================

const SequenceStep warmupSequence[] = {
  // 第 1 小节
  {KICK_MAIN, SOUND_KICK,   65, 2},
  {NOTE_E4,   SOUND_MELODY, 60, 0},
  {REST,      SOUND_REST,    0, 0},
  {NOTE_G4,   SOUND_MELODY, 60, 0},

  {KICK_LOW,  SOUND_KICK,   55, 1},
  {NOTE_C5,   SOUND_MELODY, 65, 0},
  {REST,      SOUND_REST,    0, 0},
  {NOTE_G4,   SOUND_MELODY, 55, 0},

  {KICK_MAIN, SOUND_KICK,   65, 2},
  {NOTE_E4,   SOUND_MELODY, 55, 0},
  {REST,      SOUND_REST,    0, 0},
  {NOTE_D4,   SOUND_MELODY, 60, 0},

  {KICK_LOW,  SOUND_KICK,   55, 1},
  {NOTE_G4,   SOUND_MELODY, 60, 0},
  {NOTE_A4,   SOUND_MELODY, 45, 0},
  {REST,      SOUND_REST,    0, 0},

  // 第 2 小节
  {KICK_MAIN, SOUND_KICK,   65, 2},
  {NOTE_F4,   SOUND_MELODY, 60, 0},
  {REST,      SOUND_REST,    0, 0},
  {NOTE_A4,   SOUND_MELODY, 60, 0},

  {KICK_LOW,  SOUND_KICK,   55, 1},
  {NOTE_C5,   SOUND_MELODY, 65, 0},
  {REST,      SOUND_REST,    0, 0},
  {NOTE_A4,   SOUND_MELODY, 55, 0},

  {KICK_MAIN, SOUND_KICK,   65, 2},
  {NOTE_G4,   SOUND_MELODY, 60, 0},
  {REST,      SOUND_REST,    0, 0},
  {NOTE_E4,   SOUND_MELODY, 55, 0},

  {KICK_LOW,  SOUND_KICK,   55, 1},
  {NOTE_D4,   SOUND_MELODY, 50, 0},
  {NOTE_G4,   SOUND_MELODY, 55, 0},
  {NOTE_C5,   SOUND_MELODY, 75, 1}
};


// ============================================================
// 慢跑音乐
//
// 135 BPM
// 强调低音推进
// 鼓点对应每一个主拍
// ============================================================

const SequenceStep jogSequence[] = {
  // 第 1 小节
  {KICK_MAIN, SOUND_KICK,   70, 2},
  {BASS_C2,   SOUND_BASS,   60, 0},
  {NOTE_G4,   SOUND_MELODY, 45, 0},
  {BASS_G2,   SOUND_BASS,   55, 0},

  {KICK_LOW,  SOUND_KICK,   65, 1},
  {BASS_C2,   SOUND_BASS,   55, 0},
  {NOTE_E4,   SOUND_MELODY, 45, 0},
  {NOTE_G4,   SOUND_MELODY, 50, 0},

  {KICK_MAIN, SOUND_KICK,   70, 2},
  {BASS_A2,   SOUND_BASS,   55, 0},
  {NOTE_C5,   SOUND_MELODY, 50, 1},
  {BASS_G2,   SOUND_BASS,   55, 0},

  {KICK_LOW,  SOUND_KICK,   65, 1},
  {NOTE_A4,   SOUND_MELODY, 45, 0},
  {NOTE_G4,   SOUND_MELODY, 45, 0},
  {REST,      SOUND_REST,    0, 0},

  // 第 2 小节
  {KICK_MAIN, SOUND_KICK,   70, 2},
  {BASS_F2,   SOUND_BASS,   60, 0},
  {NOTE_A4,   SOUND_MELODY, 45, 0},
  {BASS_C2,   SOUND_BASS,   55, 0},

  {KICK_LOW,  SOUND_KICK,   65, 1},
  {NOTE_C5,   SOUND_MELODY, 50, 0},
  {NOTE_A4,   SOUND_MELODY, 45, 0},
  {BASS_G2,   SOUND_BASS,   55, 0},

  {KICK_MAIN, SOUND_KICK,   70, 2},
  {BASS_G2,   SOUND_BASS,   60, 0},
  {NOTE_D5,   SOUND_MELODY, 50, 1},
  {NOTE_B4,   SOUND_MELODY, 45, 0},

  {KICK_LOW,  SOUND_KICK,   65, 1},
  {BASS_G2,   SOUND_BASS,   55, 0},
  {NOTE_A4,   SOUND_MELODY, 45, 0},
  {NOTE_G4,   SOUND_MELODY, 70, 1}
};


// ============================================================
// 正常跑音乐
//
// 160 BPM
// 鼓点更强
// 加入更多低音切分
// ============================================================

const SequenceStep runSequence[] = {
  // 第 1 小节
  {KICK_HARD, SOUND_KICK,   78, 2},
  {BASS_C2,   SOUND_BASS,   55, 0},
  {NOTE_C5,   SOUND_MELODY, 40, 1},
  {BASS_G2,   SOUND_BASS,   50, 0},

  {KICK_MAIN, SOUND_KICK,   70, 1},
  {BASS_C2,   SOUND_BASS,   50, 0},
  {NOTE_E5,   SOUND_MELODY, 42, 0},
  {BASS_G2,   SOUND_BASS,   50, 0},

  {KICK_HARD, SOUND_KICK,   78, 2},
  {BASS_A2,   SOUND_BASS,   55, 0},
  {NOTE_G5,   SOUND_MELODY, 42, 1},
  {BASS_E2,   SOUND_BASS,   50, 0},

  {KICK_MAIN, SOUND_KICK,   70, 1},
  {NOTE_E5,   SOUND_MELODY, 40, 0},
  {NOTE_D5,   SOUND_MELODY, 40, 0},
  {REST,      SOUND_REST,    0, 0},

  // 第 2 小节
  {KICK_HARD, SOUND_KICK,   78, 2},
  {BASS_F2,   SOUND_BASS,   55, 0},
  {NOTE_C5,   SOUND_MELODY, 40, 1},
  {BASS_A2,   SOUND_BASS,   50, 0},

  {KICK_MAIN, SOUND_KICK,   70, 1},
  {BASS_F2,   SOUND_BASS,   50, 0},
  {NOTE_F5,   SOUND_MELODY, 42, 0},
  {NOTE_E5,   SOUND_MELODY, 38, 0},

  {KICK_HARD, SOUND_KICK,   78, 2},
  {BASS_G2,   SOUND_BASS,   55, 0},
  {NOTE_D5,   SOUND_MELODY, 42, 1},
  {BASS_G2,   SOUND_BASS,   50, 0},

  {KICK_MAIN, SOUND_KICK,   70, 1},
  {NOTE_G5,   SOUND_MELODY, 40, 0},
  {NOTE_E5,   SOUND_MELODY, 40, 0},
  {NOTE_C5,   SOUND_MELODY, 65, 1}
};


// ============================================================
// 冲刺音乐
//
// 185 BPM
// 主拍强鼓 + 中间加双鼓
// 旋律缩短，低音和节奏优先
// ============================================================

const SequenceStep sprintSequence[] = {
  // 第 1 小节
  {KICK_HARD, SOUND_KICK,   82, 2},
  {BASS_C2,   SOUND_BASS,   45, 0},
  {KICK_LOW,  SOUND_KICK,   42, 1},
  {NOTE_G5,   SOUND_MELODY, 35, 0},

  {KICK_HARD, SOUND_KICK,   78, 2},
  {BASS_C2,   SOUND_BASS,   45, 0},
  {NOTE_C5,   SOUND_MELODY, 35, 0},
  {KICK_LOW,  SOUND_KICK,   40, 1},

  {KICK_HARD, SOUND_KICK,   82, 2},
  {BASS_A2,   SOUND_BASS,   45, 0},
  {KICK_LOW,  SOUND_KICK,   42, 1},
  {NOTE_A5,   SOUND_MELODY, 35, 0},

  {KICK_HARD, SOUND_KICK,   78, 2},
  {NOTE_G5,   SOUND_MELODY, 35, 0},
  {NOTE_E5,   SOUND_MELODY, 35, 0},
  {KICK_LOW,  SOUND_KICK,   40, 1},

  // 第 2 小节
  {KICK_HARD, SOUND_KICK,   82, 2},
  {BASS_F2,   SOUND_BASS,   45, 0},
  {KICK_LOW,  SOUND_KICK,   42, 1},
  {NOTE_F5,   SOUND_MELODY, 35, 0},

  {KICK_HARD, SOUND_KICK,   78, 2},
  {BASS_G2,   SOUND_BASS,   45, 0},
  {NOTE_D5,   SOUND_MELODY, 35, 0},
  {KICK_LOW,  SOUND_KICK,   40, 1},

  {KICK_HARD, SOUND_KICK,   82, 2},
  {BASS_G2,   SOUND_BASS,   45, 0},
  {KICK_LOW,  SOUND_KICK,   42, 1},
  {NOTE_G5,   SOUND_MELODY, 35, 0},

  {KICK_HARD, SOUND_KICK,   78, 2},
  {NOTE_A5,   SOUND_MELODY, 35, 0},
  {NOTE_G5,   SOUND_MELODY, 35, 0},
  {NOTE_C5,   SOUND_MELODY, 60, 1}
};


// ============================================================
// 数组长度
// ============================================================

constexpr uint16_t WARMUP_LENGTH =
  sizeof(warmupSequence) / sizeof(warmupSequence[0]);

constexpr uint16_t JOG_LENGTH =
  sizeof(jogSequence) / sizeof(jogSequence[0]);

constexpr uint16_t RUN_LENGTH =
  sizeof(runSequence) / sizeof(runSequence[0]);

constexpr uint16_t SPRINT_LENGTH =
  sizeof(sprintSequence) / sizeof(sprintSequence[0]);


// ============================================================
// 接收到的数据
// ============================================================

int receivedBPM = 90;
int currentSPM = 0;
int currentIntensity = 0;

uint32_t currentStepCounter = 0;
uint32_t previousStepCounter = 0;


// ============================================================
// 音序器状态
// ============================================================

uint16_t sequenceIndex = 0;

unsigned long stepStartTime = 0;
unsigned long stepDuration = 150;
unsigned long soundEndTime = 0;

bool sequenceRunning = false;
bool soundStopped = false;
bool ledDimmed = false;


// ============================================================
// 串口缓冲区
// ============================================================

char serialBuffer[64];
uint8_t serialIndex = 0;


// ============================================================
// 静止呼吸灯
// ============================================================

unsigned long lastIdleUpdate = 0;
uint16_t idlePhase = 0;


// ============================================================
// 模式名称
// ============================================================

const char* getModeName(RunningMode mode) {
  switch (mode) {
    case MODE_IDLE:
      return "IDLE";

    case MODE_WARMUP:
      return "WARMUP";

    case MODE_JOG:
      return "JOG";

    case MODE_RUN:
      return "RUN";

    case MODE_SPRINT:
      return "SPRINT";
  }

  return "UNKNOWN";
}


// ============================================================
// 根据 SPM 判断模式
// ============================================================

RunningMode detectModeFromSPM(int spm) {
  if (spm < 70) {
    return MODE_IDLE;
  }

  if (spm < 120) {
    return MODE_WARMUP;
  }

  if (spm < 150) {
    return MODE_JOG;
  }

  if (spm < 175) {
    return MODE_RUN;
  }

  return MODE_SPRINT;
}


// ============================================================
// 模式对应音乐 BPM
//
// 鼓点每四个十六分音符出现一次，
// 所以主鼓点速度就是这里的 BPM。
// ============================================================

int getModeBPM(RunningMode mode) {
  switch (mode) {
    case MODE_IDLE:
      return 90;

    case MODE_WARMUP:
      return 105;

    case MODE_JOG:
      return 135;

    case MODE_RUN:
      return 160;

    case MODE_SPRINT:
      return 185;
  }

  return 90;
}


// ============================================================
// 获取当前音序
// ============================================================

const SequenceStep* getCurrentSequence() {
  switch (currentMode) {
    case MODE_WARMUP:
      return warmupSequence;

    case MODE_JOG:
      return jogSequence;

    case MODE_RUN:
      return runSequence;

    case MODE_SPRINT:
      return sprintSequence;

    default:
      return nullptr;
  }
}


uint16_t getCurrentSequenceLength() {
  switch (currentMode) {
    case MODE_WARMUP:
      return WARMUP_LENGTH;

    case MODE_JOG:
      return JOG_LENGTH;

    case MODE_RUN:
      return RUN_LENGTH;

    case MODE_SPRINT:
      return SPRINT_LENGTH;

    default:
      return 0;
  }
}


// ============================================================
// 切换音乐状态
// ============================================================

void updateRunningMode() {
  unsigned long now = millis();

  RunningMode detectedMode =
    detectModeFromSPM(currentSPM);

  if (detectedMode == currentMode) {
    candidateMode = currentMode;
    candidateModeStartTime = 0;
    return;
  }

  // 切换后冷却
  if (
    lastModeChangeTime != 0 &&
    now - lastModeChangeTime < MODE_COOLDOWN_MS
  ) {
    return;
  }

  // 新的候选状态
  if (detectedMode != candidateMode) {
    candidateMode = detectedMode;
    candidateModeStartTime = now;

    Serial.print("候选音乐模式：");
    Serial.println(getModeName(candidateMode));

    return;
  }

  // 连续保持 5 秒
  if (
    candidateModeStartTime != 0 &&
    now - candidateModeStartTime >= MODE_CONFIRM_TIME_MS
  ) {
    currentMode = candidateMode;

    sequenceIndex = 0;
    sequenceRunning = false;

    noTone(SPEAKER_PIN);

    lastModeChangeTime = now;
    candidateModeStartTime = 0;

    Serial.print("音乐切换为：");
    Serial.print(getModeName(currentMode));

    Serial.print(" | BPM：");
    Serial.println(getModeBPM(currentMode));
  }
}


// ============================================================
// LED 工具
// ============================================================

void setAllLEDs(uint32_t color) {
  for (uint16_t i = 0; i < LED_COUNT; i++) {
    strip.setPixelColor(i, color);
  }

  strip.show();
}


uint16_t getModeHue() {
  switch (currentMode) {
    case MODE_WARMUP:
      return 8000;

    case MODE_JOG:
      return 23000;

    case MODE_RUN:
      return 39000;

    case MODE_SPRINT:
      return 62000;

    default:
      return 35000;
  }
}


// ============================================================
// 当前事件灯光
// ============================================================

void showSequenceLED(const SequenceStep &event) {
  uint8_t brightness;

  if (event.soundType == SOUND_KICK) {
    // 鼓点强闪
    brightness = map(
      currentIntensity,
      0,
      100,
      150,
      255
    );

    setAllLEDs(
      strip.Color(
        brightness,
        brightness / 5,
        0
      )
    );
  }
  else if (event.soundType == SOUND_BASS) {
    brightness = map(
      currentIntensity,
      0,
      100,
      70,
      180
    );

    setAllLEDs(
      strip.Color(
        brightness / 5,
        0,
        brightness
      )
    );
  }
  else if (event.soundType == SOUND_MELODY) {
    brightness = map(
      currentIntensity,
      0,
      100,
      50,
      170
    );

    uint16_t hue =
      getModeHue() +
      sequenceIndex * 900UL;

    setAllLEDs(
      strip.gamma32(
        strip.ColorHSV(
          hue,
          255,
          brightness
        )
      )
    );
  }
  else {
    setAllLEDs(strip.Color(0, 0, 0));
  }

  ledDimmed = false;
}


// ============================================================
// 灯光衰减
// ============================================================

void dimLEDs() {
  for (uint16_t i = 0; i < LED_COUNT; i++) {
    uint32_t color = strip.getPixelColor(i);

    uint8_t red = (color >> 16) & 0xFF;
    uint8_t green = (color >> 8) & 0xFF;
    uint8_t blue = color & 0xFF;

    strip.setPixelColor(
      i,
      red / 7,
      green / 7,
      blue / 7
    );
  }

  strip.show();
  ledDimmed = true;
}


// ============================================================
// 真实步伐额外闪光
// ============================================================

void flashForRealStep() {
  uint8_t brightness = map(
    currentIntensity,
    0,
    100,
    70,
    220
  );

  setAllLEDs(
    strip.Color(
      brightness,
      brightness,
      brightness
    )
  );
}


// ============================================================
// 静止呼吸灯
// ============================================================

void updateIdleLighting() {
  unsigned long now = millis();

  if (now - lastIdleUpdate < 25) {
    return;
  }

  lastIdleUpdate = now;

  uint8_t brightness;

  if (idlePhase < 256) {
    brightness = idlePhase;
  }
  else {
    brightness = 511 - idlePhase;
  }

  brightness /= 5;

  setAllLEDs(
    strip.Color(
      0,
      brightness,
      brightness
    )
  );

  idlePhase++;

  if (idlePhase >= 512) {
    idlePhase = 0;
  }
}


// ============================================================
// 启动当前音序事件
// ============================================================

void startSequenceStep() {
  if (currentMode == MODE_IDLE) {
    noTone(SPEAKER_PIN);
    sequenceRunning = false;
    return;
  }

  const SequenceStep* sequence =
    getCurrentSequence();

  uint16_t length =
    getCurrentSequenceLength();

  if (sequence == nullptr || length == 0) {
    return;
  }

  const SequenceStep &event =
    sequence[sequenceIndex];

  int musicBPM =
    getModeBPM(currentMode);

  // 一个十六分音符格的时长
  stepDuration =
    60000UL /
    musicBPM /
    4UL;

  if (stepDuration < 60) {
    stepDuration = 60;
  }

  unsigned long soundDuration =
    stepDuration *
    event.gatePercent /
    100UL;

  stepStartTime = millis();
  soundEndTime = stepStartTime + soundDuration;

  soundStopped = false;
  ledDimmed = false;
  sequenceRunning = true;

  if (
    event.soundType == SOUND_REST ||
    event.frequency == REST
  ) {
    noTone(SPEAKER_PIN);
    soundStopped = true;
  }
  else {
    tone(
      SPEAKER_PIN,
      event.frequency
    );
  }

  showSequenceLED(event);
}


// ============================================================
// 更新音序器
// ============================================================

void updateMusic() {
  if (currentMode == MODE_IDLE) {
    noTone(SPEAKER_PIN);
    updateIdleLighting();
    return;
  }

  if (!sequenceRunning) {
    startSequenceStep();
    return;
  }

  unsigned long now = millis();

  // 到达 gate 结束时间，停止当前声音
  if (!soundStopped && now >= soundEndTime) {
    noTone(SPEAKER_PIN);
    soundStopped = true;
  }

  // 事件后半段灯光衰减
  if (
    !ledDimmed &&
    now - stepStartTime >= stepDuration * 58UL / 100UL
  ) {
    dimLEDs();
  }

  // 进入下一个十六分音符格
  if (now - stepStartTime >= stepDuration) {
    sequenceIndex++;

    if (
      sequenceIndex >=
      getCurrentSequenceLength()
    ) {
      sequenceIndex = 0;
    }

    startSequenceStep();
  }
}


// ============================================================
// 解析 ESP32 数据
// ============================================================

void parsePacket(char *packet) {
  if (
    packet[0] != 'P' ||
    packet[1] != ','
  ) {
    return;
  }

  char *token = strtok(packet, ",");

  if (token == nullptr) {
    return;
  }

  // BPM
  token = strtok(nullptr, ",");

  if (token == nullptr) {
    return;
  }

  receivedBPM = atoi(token);

  // SPM
  token = strtok(nullptr, ",");

  if (token == nullptr) {
    return;
  }

  int newSPM = atoi(token);

  // 强度
  token = strtok(nullptr, ",");

  if (token == nullptr) {
    return;
  }

  int newIntensity = atoi(token);

  // 步数
  token = strtok(nullptr, ",");

  if (token == nullptr) {
    return;
  }

  uint32_t newStepCounter =
    strtoul(token, nullptr, 10);

  currentSPM = constrain(
    newSPM,
    0,
    250
  );

  currentIntensity = constrain(
    newIntensity,
    0,
    100
  );

  currentStepCounter =
    newStepCounter;

  updateRunningMode();

  if (
    currentStepCounter !=
    previousStepCounter
  ) {
    flashForRealStep();

    previousStepCounter =
      currentStepCounter;
  }

  Serial.print("SPM: ");
  Serial.print(currentSPM);

  Serial.print(" | Music: ");
  Serial.print(getModeName(currentMode));

  Serial.print(" | Music BPM: ");
  Serial.print(getModeBPM(currentMode));

  Serial.print(" | Candidate: ");
  Serial.println(getModeName(candidateMode));
}


// ============================================================
// 接收 ESP32 数据
// ============================================================

void receiveESP32Data() {
  while (espSerial.available() > 0) {
    char incoming =
      static_cast<char>(espSerial.read());

    if (incoming == '\n') {
      serialBuffer[serialIndex] = '\0';

      if (serialIndex > 0) {
        parsePacket(serialBuffer);
      }

      serialIndex = 0;
    }
    else if (incoming != '\r') {
      if (
        serialIndex <
        sizeof(serialBuffer) - 1
      ) {
        serialBuffer[serialIndex] =
          incoming;

        serialIndex++;
      }
      else {
        serialIndex = 0;
      }
    }
  }
}


// ============================================================
// SETUP
// ============================================================

void setup() {
  Serial.begin(115200);
  espSerial.begin(9600);

  pinMode(SPEAKER_PIN, OUTPUT);

  strip.begin();
  strip.clear();
  strip.setBrightness(255);
  strip.show();

  currentMode = MODE_IDLE;
  candidateMode = MODE_IDLE;

  Serial.println();
  Serial.println("================================");
  Serial.println(" PacePilot Rhythmic Music V2");
  Serial.println("================================");
  Serial.println("D11 = ESP32 RX");
  Serial.println("D7  = Speaker");
  Serial.println("D5  = LED Strip");
  Serial.println();
  Serial.println("低音鼓点已启用。");
  Serial.println("鼓点跟随已确认的跑步速度档位。");
}


// ============================================================
// LOOP
// ============================================================

void loop() {
  receiveESP32Data();
  updateMusic();
}