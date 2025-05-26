#pragma once
#include "Script.h"
#include "Globals.h"

class SplineComponent;
class GameObject;

class MoveGOInSpline : public Script
{
public:
    MoveGOInSpline(GameObject* parent);

    bool Init();
    void Update(float deltaTime) override;

private:
    SplineComponent* FindSpline();

private:
    UID splineGO = INVALID_UID;
    float speed  = 0.1f;
    bool loop    = false;
    
    SplineComponent* spline = nullptr;
    float t                 = 0.0f;

    bool pingPong           = true;
    bool goingForward       = true;

};
