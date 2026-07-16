/*
 * BeatSync DFPlayer 控制模块实现
 */
#include "player.h"
#include "config.h"
#include <DFRobotDFPlayerMini.h>

DFRobotDFPlayerMini dfPlayer;

void MusicPlayer::begin() {
  Serial1.begin(DFPLAYER_BAUD, SERIAL_8N1, PIN_UART_RX, PIN_UART_TX);

  if (!dfPlayer.begin(Serial1, true, true)) {
    Serial.println("[PLAYER] DFPlayer init FAILED!");
    return;
  }

  dfPlayer.volume(DFPLAYER_VOL_INIT);
  dfPlayer.play(1);

  volume = DFPLAYER_VOL_INIT;
  currentTrack = 1;
  playing = true;

  Serial.println("[PLAYER] DFPlayer ready");
}

void MusicPlayer::update() {
  // 检查 DFPlayer 状态（如有需要）
}

void MusicPlayer::play(uint16_t track) {
  dfPlayer.play(track);
  currentTrack = track;
  playing = true;
  Serial.printf("[PLAYER] Play track %d\n", track);
}

void MusicPlayer::next() {
  currentTrack++;
  dfPlayer.next();
  Serial.printf("[PLAYER] Next -> track %d\n", currentTrack);
}

void MusicPlayer::prev() {
  if (currentTrack > 1) currentTrack--;
  dfPlayer.previous();
  Serial.printf("[PLAYER] Prev -> track %d\n", currentTrack);
}

void MusicPlayer::pause() {
  dfPlayer.pause();
  playing = false;
  Serial.println("[PLAYER] Paused");
}

void MusicPlayer::resume() {
  dfPlayer.start();
  playing = true;
  Serial.println("[PLAYER] Resumed");
}

void MusicPlayer::togglePlayPause() {
  if (playing) pause();
  else resume();
}

void MusicPlayer::setVolume(uint8_t vol) {
  if (vol > DFPLAYER_VOL_MAX) vol = DFPLAYER_VOL_MAX;
  dfPlayer.volume(vol);
  volume = vol;
  Serial.printf("[PLAYER] Volume = %d\n", vol);
}

void MusicPlayer::volumeUp() {
  if (volume < DFPLAYER_VOL_MAX) {
    setVolume(volume + 1);
  }
}

void MusicPlayer::volumeDown() {
  if (volume > 0) {
    setVolume(volume - 1);
  }
}

uint8_t MusicPlayer::getVolume() {
  return volume;
}

uint16_t MusicPlayer::getCurrentTrack() {
  return currentTrack;
}

bool MusicPlayer::isPlaying() {
  return playing;
}
