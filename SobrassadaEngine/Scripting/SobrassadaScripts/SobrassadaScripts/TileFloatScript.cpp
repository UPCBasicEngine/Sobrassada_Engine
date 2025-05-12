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

void TileFloatScript::Inspector()
{
}

void TileFloatScript::Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator)
{
    targetState.AddMember("Speed", speed, allocator);
}

void TileFloatScript::Load(const rapidjson::Value& initialState)
{
    if (initialState.HasMember("Speed") && initialState["Speed"].IsFloat())
    {
        speed = initialState["Speed"].GetFloat();
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
    initialY = parent->GetLocalTransform().TranslatePart().y;

    GLOG("Initiating TileFloatScript");
    return true;
}
void TileFloatScript::Update(float deltaTime)
{
    if (!character) return;

    const float distance = character->GetLastPosition().Distance(parent->GetPosition());

    if (!isActive && distance <= 6.0f)
    {
        isActive = true;
    }

    if (!isActive) return;

    float3 currentPos     = parent->GetLocalTransform().TranslatePart();
    float4x4 newTransform = parent->GetLocalTransform();

    float targetY;

    if (distance <= 5.0f)
    {
        targetY = initialY;
        if (currentPos.y < initialY)
        {
            float riseStep = speed * deltaTime * 5;
            currentPos.y   = min(currentPos.y + riseStep, initialY);
        }
    }
    else
    {

        targetY = -3.0f;
        if (currentPos.y > targetY)
        {
            float lowerStep = speed * deltaTime * 5;
            currentPos.y    = max(currentPos.y - lowerStep, targetY);
        }

        if (fabs(currentPos.y - targetY) < 0.01f && distance > 10.0f)
        {
            isActive = false;
        }
    }

    newTransform.SetTranslatePart(currentPos);
    parent->SetLocalTransform(newTransform);
    parent->UpdateTransformForGOBranch();
}