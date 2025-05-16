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
    fields.push_back({"Speed", InspectorField::FieldType::Float, &speed, 0.0f, 100.0f});
    fields.push_back({"Starting Position", InspectorField::FieldType::Vec3, &startPosition, 0.0, 100.0f});
    fields.push_back({"Starting Rotation", InspectorField::FieldType::Vec3, &startRotation, 0.0, 100.0f});
    fields.push_back({"Starting Scale", InspectorField::FieldType::Vec3, &startScale, 0.0, 100.0f});
}

void TileFloatScript::Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator)
{
    targetState.AddMember("Speed", speed, allocator);

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
    if (initialState.HasMember("Speed") && initialState["Speed"].IsFloat()) speed = initialState["Speed"].GetFloat();

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
    character->GetLastPosition();
    if (!character)
    {
        GLOG("CharacterController component not found for CuChulainn");
        return false;
    }

    // get final (correct) position, and move the tile to start (rotated, moved and scaled) position
    finalPosition           = parent->GetLocalTransform().TranslatePart();
    finalRotation           = Quat(parent->GetLocalTransform().RotatePart());
    finalScale              = parent->GetLocalTransform().GetScale();

    Quat startQuat          = Quat::FromEulerXYZ(startRotation.x, startRotation.y, startRotation.z);

    float4x4 startTransform = float4x4::FromTRS(startPosition, startQuat, startScale);

    parent->SetLocalTransform(startTransform);

    GLOG("Initiating TileFloatScript");
    return true;
}

void TileFloatScript::Update(float deltaTime)
{
    if (!character) return;

    float distance = character->GetLastPosition().Distance(parent->GetPosition());

    // Set active when in range
    isActive       = (distance <= 10.0f); // Adjust this threshold as needed
    if (!isActive) return;

    bool goingToFinal   = distance <= 6.0f;

    float factor        = 1.0f / max(distance, 0.1f);
    float adjustedSpeed = speed * deltaTime * factor;

    float3 currentT     = parent->GetPosition();
    float3 currentS     = parent->GetScale();

    Quat currentQuat(parent->GetLocalTransform().RotatePart());

    float3 targetT = goingToFinal ? finalPosition : startPosition;
    float3 targetS = goingToFinal ? finalScale : startScale;
    Quat targetQuat =
        goingToFinal ? finalRotation : Quat::FromEulerXYZ(startRotation.x, startRotation.y, startRotation.z);

    float3 newT           = Lerp(currentT, targetT, adjustedSpeed);
    float3 newS           = Lerp(currentS, targetS, adjustedSpeed);
    Quat newQuat          = Quat::Slerp(currentQuat, targetQuat, adjustedSpeed);

    float4x4 newTransform = float4x4::FromTRS(newT, newQuat, newS);
    parent->SetLocalTransform(newTransform);
    parent->UpdateTransformForGOBranch();
}