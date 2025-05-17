#include "pch.h"

#include "TileFloatScript.h"

#include "Application.h"
#include "CameraModule.h"
#include "CuChulainn.h"
#include "EditorUIModule.h"
#include "GameObject.h"
#include "ImGui.h"
#include "LibraryModule.h"
#include "Math/float4x4.h"
#include "Standalone/CharacterControllerComponent.h"

TileFloatScript::TileFloatScript(GameObject* parent) : Script(parent)
{
    fields.push_back({"Speed", InspectorField::FieldType::Int, &speed, 0, 100});
    fields.push_back({"MinDistanceToPlayer", InspectorField::FieldType::Float, &minDistanceToPlayer, 0.0f, 100.0f});
    fields.push_back({"Starting Position", InspectorField::FieldType::Vec3, &startPosition, 0.0, 100.0f});
    fields.push_back({"Starting Rotation", InspectorField::FieldType::Vec3, &startRotation, 0.0, 100.0f});
    fields.push_back({"Starting Scale", InspectorField::FieldType::Vec3, &startScale, 0.0, 100.0f});
}

void TileFloatScript::Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator)
{
    targetState.AddMember("Speed", speed, allocator);
    targetState.AddMember("MinDistanceToPlayer", minDistanceToPlayer, allocator);

    rapidjson::Value startPos(rapidjson::kArrayType);
    startPos.PushBack(startPosition.x, allocator);
    startPos.PushBack(startPosition.y, allocator);
    startPos.PushBack(startPosition.z, allocator);
    targetState.AddMember("StartPosition", startPos, allocator);

    rapidjson::Value startRot(rapidjson::kArrayType);
    startRot.PushBack(startRotation.x, allocator);
    startRot.PushBack(startRotation.y, allocator);
    startRot.PushBack(startRotation.z, allocator);
    targetState.AddMember("StartRotation", startRot, allocator);

    rapidjson::Value startScaleVal(rapidjson::kArrayType);
    startScaleVal.PushBack(startScale.x, allocator);
    startScaleVal.PushBack(startScale.y, allocator);
    startScaleVal.PushBack(startScale.z, allocator);
    targetState.AddMember("StartScale", startScaleVal, allocator);
}

void TileFloatScript::Load(const rapidjson::Value& initialState)
{
    if (initialState.HasMember("Speed") && initialState["Speed"].IsInt()) speed = initialState["Speed"].GetInt();

    if (initialState.HasMember("MinDistanceToPlayer") && initialState["MinDistanceToPlayer"].IsFloat())
        minDistanceToPlayer = initialState["MinDistanceToPlayer"].GetFloat();

    if (initialState.HasMember("StartPosition") && initialState["StartPosition"].IsArray())
    {
        const auto& arr = initialState["StartPosition"].GetArray();
        if (arr.Size() == 3) startPosition = float3(arr[0].GetFloat(), arr[1].GetFloat(), arr[2].GetFloat());
    }

    if (initialState.HasMember("StartRotation") && initialState["StartRotation"].IsArray())
    {
        const auto& arr = initialState["StartRotation"].GetArray();
        if (arr.Size() == 3) startRotation = float3(arr[0].GetFloat(), arr[1].GetFloat(), arr[2].GetFloat());
    }

    if (initialState.HasMember("StartScale") && initialState["StartScale"].IsArray())
    {
        const auto& arr = initialState["StartScale"].GetArray();
        if (arr.Size() == 3) startScale = float3(arr[0].GetFloat(), arr[1].GetFloat(), arr[2].GetFloat());
    }
}

bool TileFloatScript::Init()
{

    // get final (correct) position, and move the tile to start (rotated, moved and scaled) position
    const float4x4 originalTransform = parent->GetLocalTransform();
    finalPosition                    = originalTransform.TranslatePart();
    finalRotation                    = Quat(originalTransform.RotatePart());
    finalScale                       = originalTransform.GetScale();

    startQuat                        = Quat::FromEulerXYZ(startRotation.x, startRotation.y, startRotation.z);

    const float4x4 startTransform    = float4x4::FromTRS(startPosition, startQuat, startScale);
    currentRotationQuat              = startQuat;

    parent->SetLocalTransform(startTransform);
    parent->UpdateTransformForGOBranch();

    GLOG("Initiating TileFloatScript");
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

    const float3 currentT   = parent->GetPosition();
    const float3 currentS   = parent->GetScale();

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
        parent->UpdateTransformForGOBranch();
        return;
    }

    const float3 newT           = Lerp(currentT, targetT, adjustedSpeed);
    const float3 newS           = Lerp(currentS, targetS, adjustedSpeed);
    currentRotationQuat         = Quat::Slerp(currentRotationQuat, targetQuat, adjustedSpeed).Normalized();

    const float4x4 newTransform = float4x4::FromTRS(newT, currentRotationQuat, newS);
    parent->SetLocalTransform(newTransform);
    parent->UpdateTransformForGOBranch();
}