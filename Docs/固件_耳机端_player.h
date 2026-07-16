/*
 * BeatSync DFPlayer 控制模块
 */
#ifndef PLAYER_H
#define PLAYER_H

#include <Arduino.h>

class MusicPlayer {
public:
  void begin();
  void update();
  void play(uint16_t track);
  void next();
  void prev();
  void pause();
  void resume();
  void togglePlayPause();
  void setVolume(uint8_t vol);
  void volumeUp();
  void volumeDown();
  uint8_t getVolume();
  uint16_t getCurrentTrack();
  bool isPlaying();

private:
  uint8_t volume;
  uint16_t currentTrack;
  bool playing;
};

#endif
