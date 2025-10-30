#include "pch.h"
#include "Crow.h"

#include "Standalone/AnimationComponent.h"
#include "Standalone/AnimController.h"
#include "GameObject.h"
#include "SceneModule.h"
#include "MoveGOInSpline.h"
#include "ScriptComponent.h"
#include "Standalone/SplineComponent.h"
#include "Standalone/Audio/AudioSourceComponent.h"
#include "Standalone/Physics/CubeColliderComponent.h"
#include "ParticleSystemComponent.h"
#include "Wwise_IDs.h"

Crow::Crow(GameObject* parent)
    : Character(
          parent, 1, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, CharacterType::Crow)
{
    fields.push_back({"Disable end route?", InspectorField::FieldType::Bool, &endRouteDisable});
    fields.push_back({"VFX Particle Feathers", InspectorField::FieldType::InputText, &nameVFXFeathers});
}

bool Crow::Init()
{
    Character::Init();
    EnterState(CrowStates::IDLE);

    parentGO     = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(parent->GetParent());
    
    moveGOSpline = parent->GetComponent<ScriptComponent*>()->GetScriptByType<MoveGOInSpline>();
    if (moveGOSpline) moveGOSpline->SetEnabled(false);

    if (!nameVFXFeathers.empty())
    {
        feathers = AppEngine->GetSceneModule()
                       ->GetScene()
                       ->GetGameObjectByName(nameVFXFeathers)
                       ->GetComponent<ParticleSystemComponent*>();
        
        feathers->StopInstances();
    }

    audioComp = parent->GetComponent<AudioSourceComponent*>();

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
        {
            if (audioComp) audioComp->EmitEvent(AK::EVENTS::PLAY_SFX_CROW_DOUBLECAW);
            EnterState(CrowStates::FLY);
        } 
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
        if (feathers) feathers->Init();
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
    if (!moveGOSpline || currentState != CrowStates::FLY) return;

    spline = moveGOSpline->GetSpline();
    if (!spline) return;

    pointSplineEnd = spline->GetPointLocal(spline->GetNumPoints() - 1);

    auto comparePositions = [](const auto& a, const auto& b, float eps = 0.1f) noexcept
    { 
            return fabsf(a.x - b.x) <= eps &&
                fabsf(a.y - b.y) <= eps &&
                fabsf(a.z - b.z) <= eps;
    };

    if (comparePositions(parent->GetPosition(), pointSplineEnd))
    {
        if (!endRouteDisable)
        {
            moveGOSpline->SetEnabled(false);
            EnterState(CrowStates::IDLE);
        }
        else
        {
            if (parentGO) 
                parentGO->SetEnabled(false);
        }
    }
}

void Crow::OnCollisionEnter(GameObject* otherObject, const float3 collisionNormal, ColliderLayer layer)
{
    if (layer != ColliderLayer::PLAYER) return;

    EnterState(CrowStates::TAKE_OFF);
    stateTimer   = 0.0f;
    playerNear   = true;
    
    if (moveGOSpline) moveGOSpline->SetEnabled(true);

    parent->GetComponent<CubeColliderComponent*>()->SetEnabled(false);
}

void Crow::SetState(CrowStates next)
{
    EnterState(next);
}
