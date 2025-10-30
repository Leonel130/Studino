#include "LedAnimator.h"

LedAnimator::LedAnimator(LedControl& ledControl, unsigned long frameDuration) {
  this->lc = &ledControl;
  this->frameDurationMS = frameDuration;
  this->currentAnimation = nullptr;
  this->currentFrame = 0;
  this->lastFrameTime = 0;
  this->looping = false;
}

void LedAnimator::play(const Animation& anim) { 
  play(anim, true); 
}

void LedAnimator::play(const Animation& anim, bool loop) {
  if (this->currentAnimation == &anim) return;
  this->currentAnimation = &anim;
  this->currentFrame = 0;
  this->looping = loop;
  this->lastFrameTime = millis();
  displayCurrentFrame();
}

void LedAnimator::stop() {
  this->currentAnimation = nullptr;
  this->lc->clearDisplay(0);
}

bool LedAnimator::isRunning() const {
  return (this->currentAnimation != nullptr);
}

void LedAnimator::actualizar() {
  if (this->currentAnimation == nullptr) return;
  unsigned long ahora = millis();
  if (ahora - this->lastFrameTime >= this->frameDurationMS) {
    this->currentFrame++;
    if (this->currentFrame >= this->currentAnimation->numFrames) {
      if (this->looping) {
        this->currentFrame = 0;
      } else {
        this->currentAnimation = nullptr;
        return;
      }
    }
    displayCurrentFrame();
    this->lastFrameTime = ahora;
  }
}

void LedAnimator::displayCurrentFrame() {
  if (this->currentAnimation == nullptr) return;
  for (int row = 0; row < 8; row++) {
    byte rowData = this->currentAnimation->data[this->currentFrame * 8 + row];
    this->lc->setRow(0, row, rowData);
  }
}