#include "pch.h"

#include "TileFloatScript.h"

#include "Application.h"
#include "CameraModule.h"
#include "CuChulainn.h"
#include "EditorUIModule.h"
#include "GameObject.h"
#include "ImGui.h"
#include "LibraryModule.h"
#include "Standalone/CharacterControllerComponent.h"

#include "Math/float4x4.h"

TileFloatScript::TileFloatScript(GameObject* parent) : Script(parent)
{
    fields.push_back({"Speed", InspectorField::FieldType::Int, &speed, 0, 100});
    fields.push_back({"MinDistanceToPlayer", InspectorField::FieldType::Float, &minDistanceToPlayer, 0.0f, 100.0f});
    fields.push_back({"Starting Position", InspectorField::FieldType::Vec3, &startPosition, 0.0, 100.0f});
    fields.push_back({"Starting Rotation", InspectorField::FieldType::Vec3, &startRotation, 0.0, 100.0f});
    fields.push_back({"Starting Scale", InspectorField::FieldType::Vec3, &startScale, 0.0, 100.0f});
    fields.push_back(
        {"Set Start Transform",
         [this](Script* self)
         {
             const float4x4& currentTransform = this->parent->GetLocalTransform();

             this->startPosition              = currentTransform.TranslatePart();
             this->startRotation              = currentTransform.RotatePart().ToEulerXYZ();
             this->startScale                 = currentTransform.GetScale();
             Quat rotQuat                     = Quat(currentTransform.RotatePart());
         }}
    );
}


bool TileFloatScript::Init()
{

    // get final (correct) position, and move the tile to start (rotated, moved and scaled) position
    const float4x4& originalTransform = parent->GetLocalTransform();
    finalPosition                    = originalTransform.TranslatePart();
    finalRotation                    = Quat(originalTransform.RotatePart());
    finalScale                       = originalTransform.GetScale();

    startQuat                        = Quat::FromEulerXYZ(startRotation.x, startRotation.y, startRotation.z);

    const float4x4 startTransform    = float4x4::FromTRS(startPosition, startQuat, startScale);
    currentRotationQuat              = startQuat;

    parent->SetLocalTransform(startTransform);

    //GLOG("Initiating TileFloatScript");
    return true;
}

void TileFloatScript::Update(float deltaTime)
{
    if (!character) return;

    const float distance = character->GetLastPosition().Distance(finalPosition);

    // Disable when far away (for performance)
    isActive             = (distance <= minDistanceToPlayer * 2);
    if (!isActive) return;

    const bool goingToFinal = distance <= minDistanceToPlayer;

    float factor            = max(distance, 0.1f);
    float adjustedSpeed     = speed * deltaTime * (1.0f / factor);
    adjustedSpeed           = std::clamp(adjustedSpeed, 0.0f, 1.0f);

    const float3& currentT   = parent->GetPosition();
    const float3& currentS   = parent->GetScale();

    const float3 targetT    = goingToFinal ? finalPosition : startPosition;
    const float3 targetS    = goingToFinal ? finalScale : startScale;
    Quat targetQuat         = goingToFinal ? finalRotation : startQuat;

    // Ensure shortest path
    if (QuaternionDot(currentRotationQuat, targetQuat) < 0.0f)
    {
        targetQuat = Quat(-targetQuat.x, -targetQuat.y, -targetQuat.z, -targetQuat.w);
    }

    float dot                = QuaternionDot(currentRotationQuat, targetQuat);
    dot                      = std::clamp(dot, -1.0f, 1.0f);
    const float angle        = acos(dot) * 2.0f;

    const bool positionClose = currentT.DistanceSq(targetT) < positionThreshold;
    const bool scaleClose    = currentS.DistanceSq(targetS) < scaleThreshold;
    const bool rotationClose = angle < rotationSnapThreshold;

    // Snap to final transform if very close
    if (positionClose && scaleClose && rotationClose)
    {
        currentRotationQuat       = targetQuat;
        float4x4 snappedTransform = float4x4::FromTRS(targetT, currentRotationQuat, targetS);
        parent->SetLocalTransform(snappedTransform);
        return;
    }

    const float3 newT           = Lerp(currentT, targetT, adjustedSpeed);
    const float3 newS           = Lerp(currentS, targetS, adjustedSpeed);
    currentRotationQuat         = Quat::Slerp(currentRotationQuat, targetQuat, adjustedSpeed).Normalized();

    const float4x4 newTransform = float4x4::FromTRS(newT, currentRotationQuat, newS);
    parent->SetLocalTransform(newTransform);
}
