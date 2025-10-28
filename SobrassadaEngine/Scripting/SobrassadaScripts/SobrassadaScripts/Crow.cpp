#include "pch.h"
#include "Crow.h"

#include "Standalone/AnimationComponent.h"
#include "Standalone/AnimController.h"
#include "GameObject.h"
#include "SceneModule.h"
#include "MoveGOInSpline.h"
#include "ScriptComponent.h"
#include "Standalone/SplineComponent.h"

Crow::Crow(GameObject* parent)
    : Character(
          parent, 1, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, CharacterType::Crow)
{
}

bool Crow::Init()
{
    Character::Init();
    EnterState(CrowStates::IDLE);
    
    moveGOSpline = parent->GetComponent<ScriptComponent*>()->GetScriptByType<MoveGOInSpline>();
    if (moveGOSpline) moveGOSpline->SetEnabled(false);

    return true;
}

void Crow::Update(float deltaTime)
{
    if (isDead) return;

    HandleState(deltaTime);

    EndRoute();

    UpdateTimers(deltaTime);
}

void Crow::HandleState(float deltaTime)
{
    switch (currentState)
    {
    case CrowStates::TAKE_OFF:
        if (animComponent && playerNear && animComponent->IsFinished()) 
            EnterState(CrowStates::FLY);
        break;

    case CrowStates::IDLE:
    case CrowStates::FLY:
    case CrowStates::NONE:
    default:
        break;
    }

    stateTimer -= deltaTime;
}

void Crow::EnterState(CrowStates next)
{
    currentState = next;
    stateTimer   = 0.f;

    if (!animComponent) return;

    switch (currentState)
    {
    case CrowStates::IDLE:
        animComponent->UseTrigger(idleTriggerName.c_str());
        playerNear = false;
        break;

    case CrowStates::TAKE_OFF:
        animComponent->UseTrigger(takeOffTriggerName.c_str());
        break;

    case CrowStates::FLY:
        animComponent->UseTrigger(flyTriggerName.c_str());
        break;

    case CrowStates::NONE:
    default:
        break;
    }
}

void Crow::EndRoute()
{
    spline         = moveGOSpline->GetSpline();

    pointSplineEnd = spline->GetPointLocal(spline->GetNumPoints() - 1);

    if (!spline) return;

    auto comparePositions = [](const auto& a, const auto& b, float eps = 0.1f) noexcept
    {
        const float dx = a.x - b.x;
        const float dy = a.y - b.y;
        const float dz = a.z - b.z;

        bool samePos   = false;

        if (dx <= eps && dy <= eps && dz <= eps) samePos = true;
        return samePos;
    };

    if (comparePositions(parent->GetPosition(), pointSplineEnd)) EnterState(CrowStates::IDLE);
}

void Crow::OnCollisionEnter(GameObject* otherObject, const float3 collisionNormal, ColliderLayer layer)
{
    if (layer != ColliderLayer::PLAYER) return;

    EnterState(CrowStates::TAKE_OFF);
    stateTimer   = 0.0f;
    playerNear   = true;
    
    if (moveGOSpline) moveGOSpline->SetEnabled(true);
}

void Crow::SetState(CrowStates next)
{
    EnterState(next);
}
