#pragma once

class Timer
{
  public:
    Timer();
    ~Timer();

    virtual void Start();
    virtual float Read();
    virtual float Stop();

    float GetDeltaTime() const { return delta_time; }

    void SetTimeScale(const float new_scale) { time_scale = new_scale; }

  private:
    virtual float TicksSinceStartup() const;

    float timer;
    bool is_enabled;

    float start_time;
    float delta_time;
    float elapsed_time;
    float real_elapsed_time = 0;
    float time_scale        = 1.0f;
};

