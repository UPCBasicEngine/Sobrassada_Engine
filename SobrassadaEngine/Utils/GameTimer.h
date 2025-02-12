#pragma once

#include "EngineTimer.h"

class GameTimer: EngineTimer
{
  public:
    GameTimer();
    ~GameTimer();

    float Tick() override;
    void Pause();
    void Reset();
    float Step();

    void SetTimeScale(const float newScale) { timeScale = newScale; }

  private:
    float TicksSinceStartup() const;
    float UpdateTimes();

    bool isPaused;

    int frameCount;
    float unscaledTime;
    float unscaledDeltaTime;
    float timeScale;
};