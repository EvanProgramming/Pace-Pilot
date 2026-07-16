#include <SoftwareSerial.h>
#include <Adafruit_NeoPixel.h>

// ============================================================
// PacePilot - Arduino Music & LED Controller
//
// 实际接线：
//
// ESP32 GPIO17 TX -> Arduino D11
// ESP32 GND       -> Arduino GND
//
// Arduino D7 -> 扬声器 / 功放
// Arduino D5 -> WS2812 RGB 灯带 DIN
//
// ESP32 数据格式：
// P,BPM,SPM,INTENSITY,STEP,ZONE
//
// 例如：
// P,160,158,70,124,3
//
// 最后一个 ZONE 字段可以存在，也可以不存在。
// Arduino 主要根据 SPM 自己判断跑步状态。
// ============================================================


// ============================================================
// 引脚
// ============================================================

constexpr uint8_t ESP32_RX_PIN = 11;

// SoftwareSerial 需要定义 RX 和 TX。
// D12 不需要实际连接。
constexpr uint8_t UNUSED_TX_PIN = 12;

constexpr uint8_t SPEAKER_PIN = 7;
constexpr uint8_t LED_PIN = 5;


// ============================================================
// LED 灯带
//
// 请根据你的实际灯珠数量修改。
// ============================================================

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
// 音符频率
// ============================================================

constexpr uint16_t REST = 0;

constexpr uint16_t NOTE_C3 = 131;
constexpr uint16_t NOTE_D3 = 147;
constexpr uint16_t NOTE_E3 = 165;
constexpr uint16_t NOTE_F3 = 175;
constexpr uint16_t NOTE_G3 = 196;
constexpr uint16_t NOTE_A3 = 220;
constexpr uint16_t NOTE_B3 = 247;

constexpr uint16_t NOTE_C4  = 262;
constexpr uint16_t NOTE_D4  = 294;
constexpr uint16_t NOTE_E4  = 330;
constexpr uint16_t NOTE_F4  = 349;
constexpr uint16_t NOTE_FS4 = 370;
constexpr uint16_t NOTE_G4  = 392;
constexpr uint16_t NOTE_A4  = 440;
constexpr uint16_t NOTE_B4  = 494;

constexpr uint16_t NOTE_C5  = 523;
constexpr uint16_t NOTE_D5  = 587;
constexpr uint16_t NOTE_E5  = 659;
constexpr uint16_t NOTE_F5  = 698;
constexpr uint16_t NOTE_FS5 = 740;
constexpr uint16_t NOTE_G5  = 784;
constexpr uint16_t NOTE_A5  = 880;
constexpr uint16_t NOTE_B5  = 988;

constexpr uint16_t NOTE_C6 = 1047;


// ============================================================
// 音乐事件结构
//
// ticks：
// 1 = 十六分音符
// 2 = 八分音符
// 4 = 四分音符
// 8 = 二分音符
//
// accent：
// 0 = 普通音
// 1 = 强拍
// 2 = 超强拍
// ============================================================

struct MusicStep {
  uint16_t note;
  uint8_t ticks;
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


// 当前真正正在播放的模式
RunningMode currentMode = MODE_IDLE;


// 当前等待确认的候选模式
RunningMode candidateMode = MODE_IDLE;


// ============================================================
// 音乐模式切换时间设置
// ============================================================

// 新的速度范围必须连续保持 5 秒
constexpr unsigned long MODE_CONFIRM_TIME_MS = 5000;


// 每次真正切换音乐后，至少等待 5 秒
// 才允许再次开始下一次模式切换
constexpr unsigned long MODE_COOLDOWN_MS = 5000;


// 候选模式开始出现的时间
unsigned long candidateModeStartTime = 0;


// 上一次真正切换音乐的时间
unsigned long lastModeChangeTime = 0;


// ============================================================
// 模式 1：热身 / 快走
//
// 特点：
// - 比较轻快
// - 节奏清晰
// - 不会太激烈
// ============================================================

const MusicStep warmupPattern[] = {

  // Bar 1
  {NOTE_C4, 2, 2},
  {NOTE_E4, 2, 0},
  {NOTE_G4, 2, 1},
  {NOTE_E4, 2, 0},

  // Bar 2
  {NOTE_C4, 2, 2},
  {NOTE_D4, 2, 0},
  {NOTE_G4, 2, 1},
  {NOTE_A4, 2, 0},

  // Bar 3
  {NOTE_F4, 2, 2},
  {NOTE_A4, 2, 0},
  {NOTE_C5, 2, 1},
  {NOTE_A4, 2, 0},

  // Bar 4
  {NOTE_G4, 2, 2},
  {NOTE_E4, 2, 0},
  {NOTE_D4, 2, 1},
  {REST,    2, 0},

  // Bar 5
  {NOTE_C4, 2, 2},
  {NOTE_G4, 1, 0},
  {NOTE_A4, 1, 0},
  {NOTE_C5, 2, 1},
  {NOTE_G4, 2, 0},

  // Bar 6
  {NOTE_F4, 2, 2},
  {NOTE_E4, 1, 0},
  {NOTE_G4, 1, 0},
  {NOTE_A4, 2, 1},
  {NOTE_C5, 2, 0},

  // Bar 7
  {NOTE_G4, 2, 2},
  {NOTE_A4, 2, 0},
  {NOTE_E4, 2, 1},
  {NOTE_D4, 2, 0},

  // Bar 8
  {NOTE_C4, 2, 2},
  {NOTE_E4, 2, 0},
  {NOTE_G4, 2, 1},
  {NOTE_C5, 2, 2}
};


// ============================================================
// 模式 2：慢跑
//
// 特点：
// - 更明显的低音推进
// - 更强的节奏感
// - 更适合持续跑步
// ============================================================

const MusicStep jogPattern[] = {

  // Bar 1
  {NOTE_C3, 1, 2},
  {NOTE_G4, 1, 0},
  {NOTE_E4, 1, 1},
  {NOTE_G4, 1, 0},
  {NOTE_C4, 1, 2},
  {NOTE_E4, 1, 0},
  {NOTE_G4, 2, 1},

  // Bar 2
  {NOTE_A3, 1, 2},
  {NOTE_E4, 1, 0},
  {NOTE_A4, 1, 1},
  {NOTE_C5, 1, 0},
  {NOTE_G4, 1, 2},
  {NOTE_E4, 1, 0},
  {NOTE_D4, 2, 1},

  // Bar 3
  {NOTE_F3, 1, 2},
  {NOTE_C5, 1, 0},
  {NOTE_A4, 1, 1},
  {NOTE_C5, 1, 0},
  {NOTE_F4, 1, 2},
  {NOTE_A4, 1, 0},
  {NOTE_G4, 2, 1},

  // Bar 4
  {NOTE_G3, 1, 2},
  {NOTE_D5, 1, 0},
  {NOTE_B4, 1, 1},
  {NOTE_G4, 1, 0},
  {NOTE_D4, 1, 2},
  {NOTE_E4, 1, 0},
  {NOTE_G4, 1, 1},
  {REST,    1, 0},

  // Bar 5
  {NOTE_C3, 1, 2},
  {NOTE_E4, 1, 0},
  {NOTE_G4, 1, 1},
  {NOTE_C5, 1, 0},
  {NOTE_G3, 1, 2},
  {NOTE_A4, 1, 0},
  {NOTE_G4, 2, 1},

  // Bar 6
  {NOTE_A3, 1, 2},
  {NOTE_C5, 1, 0},
  {NOTE_E5, 1, 1},
  {NOTE_C5, 1, 0},
  {NOTE_G3, 1, 2},
  {NOTE_A4, 1, 0},
  {NOTE_E4, 2, 1},

  // Bar 7
  {NOTE_F3, 1, 2},
  {NOTE_A4, 1, 0},
  {NOTE_C5, 1, 1},
  {NOTE_A4, 1, 0},
  {NOTE_G3, 1, 2},
  {NOTE_B4, 1, 0},
  {NOTE_D5, 2, 1},

  // Bar 8
  {NOTE_C3, 1, 2},
  {NOTE_G4, 1, 0},
  {NOTE_A4, 1, 1},
  {NOTE_C5, 1, 0},
  {NOTE_E5, 1, 2},
  {NOTE_D5, 1, 0},
  {NOTE_C5, 2, 2}
};


// ============================================================
// 模式 3：正常跑
//
// 特点：
// - 更强烈
// - 更密集
// - 有短暂停顿和切分
// ============================================================

const MusicStep runPattern[] = {

  // Bar 1
  {NOTE_C3, 1, 2},
  {NOTE_G4, 1, 0},
  {NOTE_C5, 1, 1},
  {NOTE_G4, 1, 0},
  {NOTE_E4, 1, 2},
  {REST,    1, 0},
  {NOTE_G4, 1, 1},
  {NOTE_A4, 1, 0},

  // Bar 2
  {NOTE_A3, 1, 2},
  {NOTE_E5, 1, 0},
  {NOTE_C5, 1, 1},
  {NOTE_A4, 1, 0},
  {NOTE_G3, 1, 2},
  {NOTE_B4, 1, 0},
  {NOTE_D5, 1, 1},
  {REST,    1, 0},

  // Bar 3
  {NOTE_F3, 1, 2},
  {NOTE_C5, 1, 0},
  {NOTE_A4, 1, 1},
  {NOTE_C5, 1, 0},
  {NOTE_F4, 1, 2},
  {NOTE_G4, 1, 0},
  {NOTE_A4, 1, 1},
  {NOTE_C5, 1, 0},

  // Bar 4
  {NOTE_G3, 1, 2},
  {NOTE_D5, 1, 0},
  {NOTE_G5, 1, 1},
  {NOTE_D5, 1, 0},
  {NOTE_B4, 1, 2},
  {NOTE_A4, 1, 0},
  {NOTE_G4, 1, 1},
  {REST,    1, 0},

  // Bar 5
  {NOTE_C3, 1, 2},
  {NOTE_E5, 1, 0},
  {NOTE_G5, 1, 1},
  {NOTE_E5, 1, 0},
  {NOTE_C5, 1, 2},
  {NOTE_G4, 1, 0},
  {NOTE_A4, 1, 1},
  {NOTE_C5, 1, 0},

  // Bar 6
  {NOTE_A3, 1, 2},
  {NOTE_C5, 1, 0},
  {NOTE_E5, 1, 1},
  {NOTE_G5, 1, 0},
  {NOTE_E5, 1, 2},
  {NOTE_D5, 1, 0},
  {NOTE_C5, 1, 1},
  {REST,    1, 0},

  // Bar 7
  {NOTE_F3, 1, 2},
  {NOTE_A4, 1, 0},
  {NOTE_C5, 1, 1},
  {NOTE_E5, 1, 0},
  {NOTE_G3, 1, 2},
  {NOTE_B4, 1, 0},
  {NOTE_D5, 1, 1},
  {NOTE_G5, 1, 0},

  // Bar 8
  {NOTE_C3, 1, 2},
  {NOTE_G4, 1, 0},
  {NOTE_C5, 1, 1},
  {NOTE_E5, 1, 0},
  {NOTE_G5, 1, 2},
  {NOTE_E5, 1, 0},
  {NOTE_C5, 2, 2}
};


// ============================================================
// 模式 4：冲刺
//
// 特点：
// - 最快
// - 高频
// - 短音更多
// - 更像电子游戏电子跑步节奏
// ============================================================

const MusicStep sprintPattern[] = {

  // Bar 1
  {NOTE_C4, 1, 2},
  {NOTE_C5, 1, 0},
  {NOTE_G5, 1, 1},
  {NOTE_C5, 1, 0},
  {NOTE_E5, 1, 2},
  {NOTE_G5, 1, 0},
  {NOTE_C6, 1, 1},
  {NOTE_G5, 1, 0},

  // Bar 2
  {NOTE_A3, 1, 2},
  {NOTE_A4, 1, 0},
  {NOTE_E5, 1, 1},
  {NOTE_A5, 1, 0},
  {NOTE_C5, 1, 2},
  {NOTE_E5, 1, 0},
  {NOTE_A5, 1, 1},
  {REST,    1, 0},

  // Bar 3
  {NOTE_F3, 1, 2},
  {NOTE_C5, 1, 0},
  {NOTE_F5, 1, 1},
  {NOTE_A5, 1, 0},
  {NOTE_C5, 1, 2},
  {NOTE_E5, 1, 0},
  {NOTE_G5, 1, 1},
  {NOTE_A5, 1, 0},

  // Bar 4
  {NOTE_G3, 1, 2},
  {NOTE_D5, 1, 0},
  {NOTE_G5, 1, 1},
  {NOTE_B5, 1, 0},
  {NOTE_D5, 1, 2},
  {NOTE_G5, 1, 0},
  {NOTE_A5, 1, 1},
  {REST,    1, 0},

  // Bar 5
  {NOTE_C4, 1, 2},
  {NOTE_G5, 1, 0},
  {NOTE_C6, 1, 1},
  {NOTE_G5, 1, 0},
  {NOTE_E5, 1, 2},
  {NOTE_C5, 1, 0},
  {NOTE_G5, 1, 1},
  {NOTE_C6, 1, 0},

  // Bar 6
  {NOTE_A3, 1, 2},
  {NOTE_E5, 1, 0},
  {NOTE_A5, 1, 1},
  {NOTE_E5, 1, 0},
  {NOTE_C5, 1, 2},
  {NOTE_A4, 1, 0},
  {NOTE_C5, 1, 1},
  {NOTE_E5, 1, 0},

  // Bar 7
  {NOTE_F3, 1, 2},
  {NOTE_C5, 1, 0},
  {NOTE_F5, 1, 1},
  {NOTE_A5, 1, 0},
  {NOTE_G3, 1, 2},
  {NOTE_D5, 1, 0},
  {NOTE_G5, 1, 1},
  {NOTE_B5, 1, 0},

  // Bar 8
  {NOTE_C4, 1, 2},
  {NOTE_G5, 1, 0},
  {NOTE_A5, 1, 1},
  {NOTE_C6, 1, 0},
  {NOTE_G5, 1, 2},
  {NOTE_E5, 1, 0},
  {NOTE_C5, 2, 2}
};


// ============================================================
// 各音乐长度
// ============================================================

constexpr uint16_t WARMUP_LENGTH =
  sizeof(warmupPattern) /
  sizeof(warmupPattern[0]);

constexpr uint16_t JOG_LENGTH =
  sizeof(jogPattern) /
  sizeof(jogPattern[0]);

constexpr uint16_t RUN_LENGTH =
  sizeof(runPattern) /
  sizeof(runPattern[0]);

constexpr uint16_t SPRINT_LENGTH =
  sizeof(sprintPattern) /
  sizeof(sprintPattern[0]);


// ============================================================
// 从 ESP32 收到的数据
// ============================================================

int currentBPM = 90;
int currentSPM = 0;
int currentIntensity = 0;

uint32_t currentStepCounter = 0;
uint32_t previousStepCounter = 0;


// ============================================================
// 音乐播放状态
// ============================================================

uint16_t currentMusicIndex = 0;

unsigned long noteStartTime = 0;
unsigned long noteDuration = 250;

bool notePlaying = false;
bool ledDimmed = false;


// ============================================================
// 串口缓冲区
// ============================================================

char serialBuffer[64];
uint8_t serialIndex = 0;


// ============================================================
// Idle 灯光
// ============================================================

unsigned long lastIdleUpdate = 0;
uint16_t idlePhase = 0;


// ============================================================
// 获取模式名称
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
// 根据 SPM 得到检测到的跑步状态
//
// 这里故意采用固定范围。
// 音乐不会随着几个 SPM 的波动不断变化。
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
// 根据模式得到固定 BPM
//
// 音乐不再随着实时 SPM 一直微调速度。
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
// 获取当前音乐数组
// ============================================================

const MusicStep* getCurrentPattern() {

  switch (currentMode) {

    case MODE_WARMUP:
      return warmupPattern;

    case MODE_JOG:
      return jogPattern;

    case MODE_RUN:
      return runPattern;

    case MODE_SPRINT:
      return sprintPattern;

    default:
      return nullptr;
  }
}


// ============================================================
// 获取当前音乐长度
// ============================================================

uint16_t getCurrentPatternLength() {

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
// 更新跑步音乐模式
//
// 逻辑：
//
// 1. 检测新的速度范围
//
// 2. 如果和当前音乐相同：
//    什么都不做
//
// 3. 如果不同：
//    必须连续维持 5 秒
//
// 4. 5 秒后正式切换
//
// 5. 切换后锁定 5 秒
//
// 6. 5 秒以后才能重新开始下一次切换判断
// ============================================================

void updateRunningMode() {

  unsigned long now = millis();


  RunningMode detectedMode =
    detectModeFromSPM(currentSPM);


  // ----------------------------------------------------------
  // 当前检测状态和已经播放的音乐相同
  // ----------------------------------------------------------

  if (detectedMode == currentMode) {

    candidateMode = currentMode;

    candidateModeStartTime = 0;

    return;
  }


  // ----------------------------------------------------------
  // 当前还处于切换后的 5 秒冷却期
  // ----------------------------------------------------------

  if (
    now - lastModeChangeTime <
    MODE_COOLDOWN_MS
  ) {

    return;
  }


  // ----------------------------------------------------------
  // 出现新的候选状态
  // 开始 5 秒确认
  // ----------------------------------------------------------

  if (detectedMode != candidateMode) {

    candidateMode = detectedMode;

    candidateModeStartTime = now;


    Serial.print("检测到候选模式：");

    Serial.println(
      getModeName(candidateMode)
    );

    Serial.println(
      "开始 5 秒稳定确认..."
    );


    return;
  }


  // ----------------------------------------------------------
  // 候选状态已经连续维持 5 秒
  // 正式切换音乐
  // ----------------------------------------------------------

  if (
    candidateModeStartTime != 0 &&
    now - candidateModeStartTime >=
      MODE_CONFIRM_TIME_MS
  ) {

    currentMode = candidateMode;


    // 使用这个模式对应的固定 BPM

    currentBPM =
      getModeBPM(currentMode);


    // 从新音乐开头播放

    currentMusicIndex = 0;

    notePlaying = false;

    noTone(SPEAKER_PIN);


    // 记录切换时间

    lastModeChangeTime = now;


    // 重置确认状态

    candidateModeStartTime = 0;


    Serial.println();

    Serial.print(
      "音乐模式正式切换为："
    );

    Serial.println(
      getModeName(currentMode)
    );


    Serial.print(
      "固定 BPM："
    );

    Serial.println(currentBPM);


    Serial.println(
      "进入 5 秒冷却期。"
    );

    Serial.println();
  }
}


// ============================================================
// 根据模式获得颜色基调
// ============================================================

uint16_t getModeHue() {

  switch (currentMode) {

    case MODE_WARMUP:
      return 9000;

    case MODE_JOG:
      return 25000;

    case MODE_RUN:
      return 40000;

    case MODE_SPRINT:
      return 62000;

    default:
      return 35000;
  }
}


// ============================================================
// 设置整条灯带颜色
// ============================================================

void setAllLEDs(uint32_t color) {

  for (
    uint16_t i = 0;
    i < LED_COUNT;
    i++
  ) {

    strip.setPixelColor(
      i,
      color
    );
  }

  strip.show();
}


// ============================================================
// 根据音乐状态计算颜色
// ============================================================

uint32_t calculateMusicColor(
  uint8_t accent
) {

  uint32_t hue =
    (
      static_cast<uint32_t>(
        getModeHue()
      ) +
      static_cast<uint32_t>(
        currentMusicIndex
      ) * 1100UL
    ) % 65536UL;


  uint8_t brightness;


  if (accent == 2) {

    brightness = map(
      currentIntensity,
      0,
      100,
      140,
      255
    );

  }
  else if (accent == 1) {

    brightness = map(
      currentIntensity,
      0,
      100,
      90,
      210
    );

  }
  else {

    brightness = map(
      currentIntensity,
      0,
      100,
      45,
      150
    );
  }


  uint32_t color =
    strip.ColorHSV(
      static_cast<uint16_t>(hue),
      255,
      brightness
    );


  return strip.gamma32(color);
}


// ============================================================
// 音符开始时 LED 闪烁
// ============================================================

void flashMusicLED(uint8_t accent) {

  setAllLEDs(
    calculateMusicColor(accent)
  );

  ledDimmed = false;
}


// ============================================================
// 音符后半段灯光衰减
// ============================================================

void dimLEDs() {

  uint32_t color =
    calculateMusicColor(0);


  uint8_t red =
    static_cast<uint8_t>(
      (color >> 16) & 0xFF
    );


  uint8_t green =
    static_cast<uint8_t>(
      (color >> 8) & 0xFF
    );


  uint8_t blue =
    static_cast<uint8_t>(
      color & 0xFF
    );


  red /= 8;
  green /= 8;
  blue /= 8;


  setAllLEDs(
    strip.Color(
      red,
      green,
      blue
    )
  );


  ledDimmed = true;
}


// ============================================================
// 真实步伐闪光
// ============================================================

void flashForRealStep() {

  uint8_t brightness =
    map(
      currentIntensity,
      0,
      100,
      70,
      220
    );


  uint32_t color;


  if (
    currentMode ==
    MODE_SPRINT
  ) {

    // 冲刺时使用偏红紫色强闪

    color = strip.Color(
      brightness,
      brightness / 5,
      brightness / 2
    );

  }
  else {

    // 其他状态使用白色闪光

    color = strip.Color(
      brightness,
      brightness,
      brightness
    );
  }


  setAllLEDs(color);
}


// ============================================================
// 静止状态呼吸灯
// ============================================================

void updateIdleLighting() {

  unsigned long now = millis();


  if (
    now - lastIdleUpdate < 25
  ) {

    return;
  }


  lastIdleUpdate = now;


  uint8_t brightness;


  if (idlePhase < 256) {

    brightness =
      static_cast<uint8_t>(
        idlePhase
      );

  }
  else {

    brightness =
      static_cast<uint8_t>(
        511 - idlePhase
      );
  }


  brightness /= 5;


  // 青色呼吸

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
// 开始当前音符
// ============================================================

void startCurrentNote() {

  // 静止状态不播放音乐

  if (
    currentMode ==
    MODE_IDLE
  ) {

    noTone(SPEAKER_PIN);

    notePlaying = false;

    return;
  }


  const MusicStep* pattern =
    getCurrentPattern();


  uint16_t patternLength =
    getCurrentPatternLength();


  if (
    pattern == nullptr ||
    patternLength == 0
  ) {

    return;
  }


  const MusicStep &step =
    pattern[currentMusicIndex];


  // 四分音符时长

  unsigned long quarterNoteMs =
    60000UL /
    max(currentBPM, 1);


  // 十六分音符时长

  unsigned long sixteenthNoteMs =
    quarterNoteMs / 4UL;


  noteDuration =
    sixteenthNoteMs *
    step.ticks;


  // 防止太短

  if (noteDuration < 45) {

    noteDuration = 45;
  }


  // 重拍声音持续更久

  uint8_t soundPercent;


  if (step.accent == 2) {

    soundPercent = 92;

  }
  else if (step.accent == 1) {

    soundPercent = 82;

  }
  else {

    soundPercent = 70;
  }


  unsigned long audibleDuration =
    noteDuration *
    soundPercent /
    100UL;


  // 播放或休止

  if (step.note == REST) {

    noTone(SPEAKER_PIN);

  }
  else {

    tone(
      SPEAKER_PIN,
      step.note,
      audibleDuration
    );
  }


  noteStartTime = millis();

  notePlaying = true;

  ledDimmed = false;


  if (step.note != REST) {

    flashMusicLED(
      step.accent
    );
  }
}


// ============================================================
// 持续更新音乐
//
// 完全非阻塞。
// 不使用 delay()。
// ============================================================

void updateMusic() {

  // ----------------------------------------------------------
  // 静止模式
  // ----------------------------------------------------------

  if (
    currentMode ==
    MODE_IDLE
  ) {

    noTone(SPEAKER_PIN);

    updateIdleLighting();

    return;
  }


  // ----------------------------------------------------------
  // 当前还没有播放音符
  // ----------------------------------------------------------

  if (!notePlaying) {

    startCurrentNote();

    return;
  }


  unsigned long now = millis();


  unsigned long elapsed =
    now - noteStartTime;


  // ----------------------------------------------------------
  // 音符播放到 58% 后灯光变暗
  // ----------------------------------------------------------

  if (
    !ledDimmed &&
    elapsed >=
      noteDuration *
      58UL /
      100UL
  ) {

    dimLEDs();
  }


  // ----------------------------------------------------------
  // 当前音符结束
  // ----------------------------------------------------------

  if (
    elapsed >= noteDuration
  ) {

    currentMusicIndex++;


    if (
      currentMusicIndex >=
      getCurrentPatternLength()
    ) {

      currentMusicIndex = 0;
    }


    startCurrentNote();
  }
}


// ============================================================
// 解析 ESP32 数据
//
// 支持格式：
//
// P,BPM,SPM,INTENSITY,STEP
//
// 或：
//
// P,BPM,SPM,INTENSITY,STEP,ZONE
// ============================================================

void parsePacket(char *packet) {

  if (
    packet[0] != 'P' ||
    packet[1] != ','
  ) {

    return;
  }


  char *token =
    strtok(packet, ",");


  if (token == nullptr) {

    return;
  }


  // ----------------------------------------------------------
  // BPM
  // ----------------------------------------------------------

  token =
    strtok(nullptr, ",");


  if (token == nullptr) {

    return;
  }


  int receivedBPM =
    atoi(token);


  // ----------------------------------------------------------
  // SPM
  // ----------------------------------------------------------

  token =
    strtok(nullptr, ",");


  if (token == nullptr) {

    return;
  }


  int newSPM =
    atoi(token);


  // ----------------------------------------------------------
  // Intensity
  // ----------------------------------------------------------

  token =
    strtok(nullptr, ",");


  if (token == nullptr) {

    return;
  }


  int newIntensity =
    atoi(token);


  // ----------------------------------------------------------
  // Step Counter
  // ----------------------------------------------------------

  token =
    strtok(nullptr, ",");


  if (token == nullptr) {

    return;
  }


  uint32_t newStepCounter =
    strtoul(
      token,
      nullptr,
      10
    );


  // ----------------------------------------------------------
  // Zone
  //
  // 可选字段。
  // 目前 Arduino 不依赖它。
  // ----------------------------------------------------------

  token =
    strtok(nullptr, ",");


  int receivedZone = -1;


  if (token != nullptr) {

    receivedZone =
      atoi(token);
  }


  // ----------------------------------------------------------
  // 数据检查
  // ----------------------------------------------------------

  if (
    receivedBPM < 60 ||
    receivedBPM > 220
  ) {

    return;
  }


  currentSPM =
    constrain(
      newSPM,
      0,
      250
    );


  currentIntensity =
    constrain(
      newIntensity,
      0,
      100
    );


  currentStepCounter =
    newStepCounter;


  // ----------------------------------------------------------
  // 根据当前 SPM 更新候选音乐模式
  // ----------------------------------------------------------

  updateRunningMode();


  // ----------------------------------------------------------
  // 检测真实新步伐
  // ----------------------------------------------------------

  if (
    currentStepCounter !=
    previousStepCounter
  ) {

    flashForRealStep();


    previousStepCounter =
      currentStepCounter;
  }


  // ----------------------------------------------------------
  // 调试输出
  // ----------------------------------------------------------

  Serial.print("SPM: ");

  Serial.print(currentSPM);


  Serial.print(
    " | Current Mode: "
  );

  Serial.print(
    getModeName(currentMode)
  );


  Serial.print(
    " | Candidate: "
  );

  Serial.print(
    getModeName(candidateMode)
  );


  Serial.print(
    " | BPM: "
  );

  Serial.print(currentBPM);


  Serial.print(
    " | Intensity: "
  );

  Serial.print(
    currentIntensity
  );


  Serial.print("%");


  if (receivedZone >= 0) {

    Serial.print(
      " | ESP Zone: "
    );

    Serial.print(
      receivedZone
    );
  }


  Serial.println();
}


// ============================================================
// 接收 ESP32 串口数据
//
// 非阻塞。
// ============================================================

void receiveESP32Data() {

  while (
    espSerial.available() > 0
  ) {

    char incoming =
      static_cast<char>(
        espSerial.read()
      );


    // 收到一整行

    if (incoming == '\n') {

      serialBuffer[
        serialIndex
      ] = '\0';


      if (serialIndex > 0) {

        parsePacket(
          serialBuffer
        );
      }


      serialIndex = 0;
    }


    // 忽略 \r

    else if (
      incoming != '\r'
    ) {

      if (
        serialIndex <
        sizeof(serialBuffer) - 1
      ) {

        serialBuffer[
          serialIndex
        ] = incoming;


        serialIndex++;
      }


      // 数据过长，清空

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


  pinMode(
    SPEAKER_PIN,
    OUTPUT
  );


  // 初始化 LED

  strip.begin();

  strip.clear();

  strip.setBrightness(255);

  strip.show();


  // 初始状态

  currentMode = MODE_IDLE;

  candidateMode = MODE_IDLE;

  currentBPM =
    getModeBPM(MODE_IDLE);


  Serial.println();

  Serial.println(
    "================================"
  );

  Serial.println(
    " PacePilot Music Controller"
  );

  Serial.println(
    "================================"
  );

  Serial.println();


  Serial.println(
    "Hardware:"
  );

  Serial.println(
    "D11 = ESP32 RX"
  );

  Serial.println(
    "D7  = Speaker"
  );

  Serial.println(
    "D5  = LED Strip"
  );

  Serial.println();


  Serial.println(
    "Running modes:"
  );

  Serial.println(
    "< 70 SPM   = IDLE"
  );

  Serial.println(
    "70-119     = WARMUP"
  );

  Serial.println(
    "120-149    = JOG"
  );

  Serial.println(
    "150-174    = RUN"
  );

  Serial.println(
    "175+       = SPRINT"
  );

  Serial.println();


  Serial.println(
    "Mode switching:"
  );

  Serial.println(
    "New zone must stay stable for 5 seconds."
  );

  Serial.println(
    "After switching, system locks for 5 seconds."
  );

  Serial.println();


  Serial.println(
    "Waiting for ESP32 data..."
  );
}


// ============================================================
// LOOP
// ============================================================

void loop() {

  // 接收 ESP32 数据

  receiveESP32Data();


  // 持续播放音乐与灯光

  updateMusic();
}