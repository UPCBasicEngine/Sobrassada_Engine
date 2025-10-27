#pragma once
#include "Globals.h"
#include "Script.h"

class SplineComponent;
class GameObject;

class MoveGOInSpline : public Script
{
  public:
    MoveGOInSpline(GameObject* parent);

    bool Init();
    void Update(float deltaTime) override;

    int GetLoopCounter() const { return loopCounter; }

    void SetEnabled(const bool newEnabled) { enabled = newEnabled; }

  private:
    SplineComponent* FindSpline();

  private:
    UID splineIdGO          = INVALID_UID;
    float speed             = 1.0f;
    bool isLoop             = false;
    int loopCounter         = 0;

    SplineComponent* spline = nullptr;
    float t                 = 0.0f;

    bool pingPong           = true;
    bool goingForward       = true;

    bool enabled            = true;
};
