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
    fields.push_back({"Enable Mouse Offset", InspectorField::FieldType::Bool, &aimOffsetEnabled});
    fields.push_back({"Mouse Offset Intensity", InspectorField::FieldType::Float, &aimOffsetIntensity, 0.0f, 1.0f});
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

    camera = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(parent->GetChildren()[0]);

    rng    = std::mt19937(std::random_device {}());
    dist   = std::uniform_real_distribution<float>(-1.0f, 1.0f);

    return true;
}

void CameraMovement::Update(float deltaTime)
{
    FollowTarget(deltaTime);
    if (shakeTimer > 0.0f) CameraShake(deltaTime);
}

void CameraMovement::InitAlternativeTarget(const GameObject* alternativeTarget)
{
    defaultTarget = target;
    target        = alternativeTarget;
}

void CameraMovement::ResetToDefaultTarget()
{
    target        = defaultTarget;
    defaultTarget = nullptr;
}

void CameraMovement::SetPosition(const float3& newPos)
{
    parent->SetLocalPosition(newPos);
}

void CameraMovement::StartShake(float duration, float intensity, float smoothness)
{
    shakeDuration    = duration;
    shakeTimer       = duration;
    shakeIntensity   = intensity;
    shakeSmoothness  = smoothness;
    defaultCameraPos = camera->GetPosition();
    currentOffset    = float3::zero;
}

void CameraMovement::FollowTarget(float deltaTime)
{
    if (!target) return;

    float3 desiredPosition        = target->GetGlobalTransform().TranslatePart();
    const float3& currentPosition = parent->GetGlobalTransform().TranslatePart();

    if (aimOffsetEnabled)
    {
        currentLookAhead   = 0;

        float3 mouseOffset = float3::zero;
        if (AppEngine->GetInputModule()->IsUsingKeyboard())
        {
            const float3 mouseWorldPos =
                AppEngine->GetSceneModule()->GetScene()->GetMainCamera()->ScreenPointToXZ(currentPosition.y);
            mouseOffset = (desiredPosition + (mouseWorldPos)) * 0.5f - desiredPosition;
        }
        else
        {
            const float2 stickDirection = AppEngine->GetInputModule()->GetRightStick();
            mouseOffset                 = float3(stickDirection.x, 0, stickDirection.y) * 5.0f;
        }

        desiredPosition += mouseOffset * aimOffsetIntensity;
    }
    else if (lookAheadIntensity > 0 && controller)
    {
        const float distanceToTarget = desiredPosition.Distance(currentPosition);
        if (!isFollowing && distanceToTarget < followDistanceThreshold) return;

        isFollowing      = true;

        currentLookAhead = Lerp(
            currentLookAhead, lookAheadIntensity * (controller->GetSpeed() / controller->GetMaxSpeed()),
            min(1, lookAheadSmoothness * deltaTime)
        );

        const float3& targetDir  = controller->GetFrontDirection();
        desiredPosition         += targetDir * currentLookAhead;

        if (isFollowing && distanceToTarget < 0.1f && controller->GetSpeed() < 0.1f) isFollowing = false;
    }
    finalPosition = Lerp(currentPosition, desiredPosition, min(1, smoothnessVelocity * deltaTime));

    parent->SetLocalPosition(finalPosition - parent->GetParentGlobalTransform().TranslatePart());
}

void CameraMovement::CameraShake(float deltaTime)
{
    const float progress = (shakeDuration - shakeTimer) / shakeDuration;

    const float fadeIn   = SmoothStep(0.0f, 0.25f, progress);
    const float fadeOut  = 1.0f - SmoothStep(0.75f, 1.0f, progress);
    float fade           = fadeIn < fadeOut ? fadeIn : fadeOut;
    fade                 = fade * fade * (3.0f - 2.0f * fade);

    const float x        = dist(rng) * shakeIntensity * fade;
    const float y        = dist(rng) * shakeIntensity * fade;

    float3 targetOffset  = float3(x, y, 0.0f);
    currentOffset        = shakeSmoothness > 0 ? Lerp(currentOffset, targetOffset, shakeSmoothness) : targetOffset;

    camera->SetLocalPosition(defaultCameraPos + currentOffset);

    shakeTimer -= deltaTime;

    if (shakeTimer <= 0.0f)
    {
        camera->SetLocalPosition(defaultCameraPos);
    }
}