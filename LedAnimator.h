#ifndef LED_ANIMATOR_H
#define LED_ANIMATOR_H

#include "Arduino.h"
#include "LedControl.h" // Depende de LedControl
#include "Animation.h"  // Depende de la struct Animation

class LedAnimator {
private:
  LedControl* lc;
  const Animation* currentAnimation;
  int currentFrame;
  bool looping;
  unsigned long frameDurationMS;
  unsigned long lastFrameTime;

  void displayCurrentFrame();

public:
  LedAnimator(LedControl& ledControl, unsigned long frameDuration);
  void play(const Animation& anim);
  void play(const Animation& anim, bool loop);
  void stop();
  bool isRunning() const;
  void actualizar();
};

#endif