#pragma once

#include "EngineTimer.h"
#include "Globals.h"

class GameTimer : public EngineTimer
{
  public:
    GameTimer();
    ~GameTimer() override;

    void Start() override;
    float Tick() override;

    void SOBRASADA_API_ENGINE TogglePause();
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

  private:
    bool isPaused;

    int frameCount;
    float unscaledTime;
    float unscaledDeltaTime;

    float unstoppableTime;
    float unstoppableDeltaTime;

    float referenceTime;
    float timeScale;
};