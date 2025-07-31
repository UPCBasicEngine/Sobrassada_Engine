#include "pch.h"

#include "GameObject.h"
#include "SpawnUI.h"
#include "Standalone/Physics/SphereColliderComponent.h"
#include "Standalone/UI/ImageComponent.h"

SpawnUI::SpawnUI(GameObject* parent) : Script(parent)
{
    fields.push_back({"Object UI Name", InspectorField::FieldType::InputText, &objectUIName});
    fields.push_back({"Keep after collision", InspectorField::FieldType::Bool, &keepAfterCollision});
}

bool SpawnUI::Init()
{
    trigger = parent->GetComponent<SphereColliderComponent*>();
    if (!trigger) GLOG("[WARNING] SpawnUI without sphere collider component.");

    GameObject* objectUI = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(objectUIName);
    if (objectUI)
    {
        imageUI = objectUI->GetComponent<ImageComponent*>();
        if (imageUI) imageUI->SetEnabled(false);
        else GLOG("[WARNING] No Image component in game object: %s", objectUIName.c_str());
    }
    else GLOG("[WARNING] No UI game object found by the name: %s", objectUIName.c_str());

    return true;
}

void SpawnUI::Update(float deltaTime)
{
    if (!trigger || !imageUI) return;

    if (!onCollision && !keepAfterCollision) imageUI->SetEnabled(false);

    onCollision = false;
}

void SpawnUI::OnCollision(GameObject* otherObject, const float3 collisionNormal, ColliderLayer layer)
{
    // triggers only collision with Player
    if (imageUI) imageUI->SetEnabled(true);
    onCollision = true;
}
