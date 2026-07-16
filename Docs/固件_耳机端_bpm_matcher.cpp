/*
 * BeatSync BPM 匹配切歌模块实现
 */
#include "bpm_matcher.h"
#include "config.h"
#include <FS.h>
#include <SD.h>

void BPMMatcher::begin() {
  trackCount = 0;
  currentTrackIdx = 0;
  mismatchStartTime = 0;
  mismatchActive = false;
  lastCadenceBPM = 0;

  loadBPMList();
  Serial.printf("[BPM] Loaded %d tracks\n", trackCount);
}

void BPMMatcher::loadBPMList() {
  // 从 microSD 读取 bpm_list.csv
  // 格式: track_num,filename,bpm
  // 实际实现需解析 CSV

  // 占位：硬编码示例数据
  trackList[0] = {1, 120};
  trackList[1] = {2, 130};
  trackList[2] = {3, 140};
  trackList[3] = {4, 145};
  trackList[4] = {5, 150};
  trackList[5] = {6, 155};
  trackList[6] = {7, 160};
  trackList[7] = {8, 165};
  trackList[8] = {9, 170};
  trackList[9] = {10, 175};
  trackCount = 10;

  // TODO: 实际从 SD 卡加载
  /*
  File f = SD.open(BPM_LIST_FILE);
  if (f) {
    // 解析 CSV
    f.close();
  }
  */
}

int BPMMatcher::findClosestTrack(float targetBPM) {
  int bestIdx = 0;
  float bestDiff = abs(trackList[0].bpm - targetBPM);

  for (int i = 1; i < trackCount; i++) {
    float diff = abs(trackList[i].bpm - targetBPM);
    if (diff < bestDiff) {
      bestDiff = diff;
      bestIdx = i;
    }
  }
  return bestIdx;
}

float BPMMatcher::getCurrentTrackBPM() {
  if (currentTrackIdx < trackCount) {
    return trackList[currentTrackIdx].bpm;
  }
  return 0;
}

void BPMMatcher::update(float cadenceBPM) {
  lastCadenceBPM = cadenceBPM;

  float trackBPM = getCurrentTrackBPM();
  float diff = abs(cadenceBPM - trackBPM);

  if (diff > BPM_MATCH_THRESHOLD) {
    // 步频与当前歌曲不匹配
    if (!mismatchActive) {
      mismatchActive = true;
      mismatchStartTime = millis();
    }
  } else {
    // 匹配，重置
    mismatchActive = false;
  }
}

bool BPMMatcher::shouldSwitch() {
  if (!mismatchActive) return false;

  // 持续超过阈值时间
  if (millis() - mismatchStartTime >= BPM_MATCH_DURATION) {
    mismatchActive = false;
    return true;
  }
  return false;
}

uint16_t BPMMatcher::getMatchedTrack() {
  int idx = findClosestTrack(lastCadenceBPM);
  currentTrackIdx = idx;
  Serial.printf("[BPM] Matched track %d (BPM=%.0f) for cadence=%.1f\n",
    trackList[idx].trackNum, trackList[idx].bpm, lastCadenceBPM);
  return trackList[idx].trackNum;
}
