#include "pch.h"

#include "GameObject.h"
#include "SpawnUI.h"
#include "Standalone/Physics/SphereColliderComponent.h"

SpawnUI::SpawnUI(GameObject* parent) : Script(parent)
{
    fields.push_back({"Object UI Name", InspectorField::FieldType::InputText, &objectUIName});
}

bool SpawnUI::Init()
{
    trigger = parent->GetComponent<SphereColliderComponent*>();
    if (!trigger) GLOG("[WARNING] SpawnUI without sphere collider component.");

    objectUI = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(objectUIName);
    if (objectUI) objectUI->SetEnabled(false);
    else GLOG("[WARNING] No UI game object found by the name: %s", objectUIName.c_str());

    return true;
}

void SpawnUI::Update(float deltaTime)
{
    if (!trigger || !objectUI) return;

    if (!onCollision) objectUI->SetEnabled(false);

    onCollision = false;
}

void SpawnUI::OnCollision(GameObject* otherObject, const float3& collisionNormal)
{
    // triggers only collision with Player
    objectUI->SetEnabled(true);
    onCollision = true;
}
