#pragma once

#include "EngineTimer.h"

class GameTimer : public EngineTimer
{
  public:
    GameTimer();
    ~GameTimer();

    void Start() override;
    float Tick() override;
    void TogglePause();
    void Reset();
    float Step();

    int GetFrameCount() const { return frameCount; }
    float GetUnscaledTime() const { return unscaledTime; }
    float GetUnscaledDeltaTime() const { return unscaledDeltaTime; }
    float GetTimeScale() const { return timeScale; }
    float GetReferenceTime() const { return referenceTime; }

    void SetTimeScale(const float newScale) { timeScale = newScale; }

  private:
    float TicksSinceReference() const;
    float UpdateTimes();

    bool isPaused;

    int frameCount;
    float unscaledTime;
    float unscaledDeltaTime;

    float unstoppableTime;
    float unstoppableDeltaTime;

    float referenceTime;
    float timeScale;
};