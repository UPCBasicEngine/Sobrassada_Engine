#include "pch.h"

#include "CameraMovement.h"

#include "Application.h"
#include "CameraComponent.h"
#include "EditorUIModule.h"
#include "GameObject.h"
#include "InputModule.h"
#include "Interpolation.h"
#include "Scene.h"
#include "SceneModule.h"
#include "Standalone/CharacterControllerComponent.h"

#include "Math/MathFunc.h"

CameraMovement::CameraMovement(GameObject* parent) : Script(parent)
{
    fields.push_back({"Target", InspectorField::FieldType::InputText, &targetName});
    fields.push_back({"Smoothness Velocity", InspectorField::FieldType::Float, &smoothnessVelocity, 0.0f, 20.0f});
    fields.push_back({"Enable Mouse Offset", InspectorField::FieldType::Bool, &mouseOffsetEnabled});
    fields.push_back({"Mouse Offset Intensity", InspectorField::FieldType::Float, &mouseOffsetIntensity, 0.0f, 1.0f});
    fields.push_back({"Look Ahead Intensity", InspectorField::FieldType::Float, &lookAheadIntensity, 0.0f, 10.0f});
    fields.push_back({"Look Ahead Smoothness", InspectorField::FieldType::Float, &lookAheadSmoothness, 0.0f, 20.0f});
    fields.push_back(
        {"Follow Distance Threshold", InspectorField::FieldType::Float, &followDistanceThreshold, 0.0f, 10.0f}
    );
}

bool CameraMovement::Init()
{
    const GameObject* targetObj = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(targetName);
    if (targetObj)
    {
        target     = targetObj;
        controller = target->GetComponent<CharacterControllerComponent*>();
    }

    return true;
}

void CameraMovement::Update(float deltaTime)
{
    FollowTarget(deltaTime);
}

void CameraMovement::FollowTarget(float deltaTime)
{
    if (!target) return;

    float3 desiredPosition        = target->GetGlobalTransform().TranslatePart();
    const float3& currentPosition = parent->GetGlobalTransform().TranslatePart();

    if (mouseOffsetEnabled)
    {
        currentLookAhead = 0;
        const float3 mouseWorldPos =
            AppEngine->GetSceneModule()->GetScene()->GetMainCamera()->ScreenPointToXZ(currentPosition.y);
        const float3 mouseOffset  = (desiredPosition + (mouseWorldPos)) * 0.5f - desiredPosition;
        desiredPosition          += mouseOffset * mouseOffsetIntensity;
    }
    else if (lookAheadIntensity > 0 && controller)
    {
        const float distanceToTarget = desiredPosition.Distance(currentPosition);
        if (!isFollowing && distanceToTarget < followDistanceThreshold) return;

        isFollowing      = true;

        currentLookAhead = Lerp(
            currentLookAhead, lookAheadIntensity * (controller->GetSpeed() / controller->GetMaxSpeed()),
            lookAheadSmoothness * deltaTime
        );

        const float3& targetDir  = controller->GetFrontDirection();
        desiredPosition         += targetDir * currentLookAhead;

        if (isFollowing && distanceToTarget < 0.1f && controller->GetSpeed() < 0.1f) isFollowing = false;
    }
    finalPosition = Lerp(currentPosition, desiredPosition, smoothnessVelocity * deltaTime);

    parent->SetLocalPosition(finalPosition - parent->GetParentGlobalTransform().TranslatePart());
}

void CameraMovement::SetPosition(const float3& newPos)
{
    parent->SetLocalPosition(newPos);
}
