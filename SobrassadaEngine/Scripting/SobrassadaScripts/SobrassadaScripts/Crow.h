#pragma once
#include "Character.h"

class AudioSourceComponent;
class MoveGOInSpline;
class SplineComponent;


enum class CrowStates
{
    NONE     = 0,
    IDLE     = 1,
    TAKE_OFF = 2,
    FLY      = 3,

};

class Crow : public Character
{
  public:
    Crow(GameObject* parent);
    ~Crow() noexcept override { parent = nullptr; };

    bool Init() override;
    void Update(float deltaTime) override;

    void TakeDamage(int damage) override {};
    void OnCollisionEnter(GameObject* otherObject, const float3 collisionNormal, ColliderLayer layer) override;

    void SetState(CrowStates next);

  private:
    void HandleState(float deltaTime) override;
    void EnterState(CrowStates next);
    void EndRoute();

  private:
    CrowStates currentState = CrowStates::NONE;
    GameObject* parentGO    = nullptr;

    std::string idleTriggerName     = "Idle";
    std::string takeOffTriggerName  = "Take_off";
    std::string flyTriggerName      = "Fly";

    bool playerNear                 = false;

    float stateTimer        = 0.f;
    AudioSourceComponent* audioComp = nullptr;
    MoveGOInSpline* moveGOSpline    = nullptr;
    SplineComponent* spline         = nullptr;
    float3 pointSplineEnd           = float3::zero;

    bool endRouteDisable            = false;
};
