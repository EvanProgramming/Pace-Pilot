/*
 * BeatSync BPM 匹配切歌模块
 */
#ifndef BPM_MATCHER_H
#define BPM_MATCHER_H

#include <Arduino.h>

struct TrackBPM {
  uint16_t trackNum;
  float bpm;
};

class BPMMatcher {
public:
  void begin();
  void update(float cadenceBPM);
  uint16_t getMatchedTrack();
  bool shouldSwitch();
  float getCurrentTrackBPM();

private:
  static const int MAX_TRACKS = 200;

  TrackBPM trackList[MAX_TRACKS];
  int trackCount;
  uint16_t currentTrackIdx;

  float lastCadenceBPM;
  unsigned long mismatchStartTime;
  bool mismatchActive;

  void loadBPMList();
  int findClosestTrack(float targetBPM);
};

#endif
