#include "pch.h"

#include "Application.h"
#include "CameraModule.h"
#include "EditorUIModule.h"
#include "GameObject.h"
#include "ImGui.h"
#include "Math/float4x4.h"
#include "RotateGameObject.h"

RotateGameObject::RotateGameObject(GameObject* parent) : Script(parent)
{
    // Using ImGui in the dll cause problems, so we need to call ImGui outside the dll
    fields.push_back({InspectorField::FieldType::Text, (void*)"Test"});
    fields.push_back({"Speed", InspectorField::FieldType::Float, &speed, 0.0f, 2.0f});
    fields.push_back({"Vec2 Test", InspectorField::FieldType::Vec2, &prueba});
    fields.push_back({"Color Test", InspectorField::FieldType::Color, &color});
    fields.push_back({"Target", InspectorField::FieldType::GameObject, &target});
}

bool RotateGameObject::Init()
{
    GLOG("Initiating RotationGameObject");
    return true;
}

void RotateGameObject::Update(float deltaTime)
{
    if (target != nullptr)
    {
        float4x4 newTransform = target->GetLocalTransform();
        newTransform          = newTransform * float4x4::RotateX(speed * deltaTime);
        target->SetLocalTransform(newTransform);
        target->UpdateTransformForGOBranch();
    }
    else
    {
        float4x4 newTransform = parent->GetLocalTransform();
        newTransform          = newTransform * float4x4::RotateX(speed * deltaTime);
        parent->SetLocalTransform(newTransform);
        parent->UpdateTransformForGOBranch();
    }
}
