#pragma once

class EngineTimer
{
  public:
    EngineTimer();
    ~EngineTimer();

    virtual void Start();
    virtual float Tick();

    float GetTotalTime() const { return time; }

  protected:
    virtual float TicksSinceStartup() const;

    bool isEnabled;
    float time;
    float startTime;
    float deltaTime;
};