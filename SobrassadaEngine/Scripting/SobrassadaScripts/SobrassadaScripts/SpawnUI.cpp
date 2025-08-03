#include "pch.h"

#include "GameObject.h"
#include "SpawnUI.h"
#include "Standalone/Physics/SphereColliderComponent.h"
#include "Standalone/UI/ImageComponent.h"
#include "ScriptComponent.h"
#include "CuChulainn.h"

SpawnUI::SpawnUI(GameObject* parent) : Script(parent)
{
    fields.push_back({"Object UI Name", InspectorField::FieldType::InputText, &objectUIName});
    fields.push_back({"Unlocks Ability", InspectorField::FieldType::Bool, &unlockAbility});
    fields.push_back({"Ability Name", InspectorField::FieldType::InputText, &nameAbility});
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

    if (!onCollision && !unlockAbility) imageUI->SetEnabled(false);

    onCollision = false;
}

void SpawnUI::OnCollision(GameObject* otherObject, const float3 collisionNormal, ColliderLayer layer)
{
    // triggers only collision with Player
    if (imageUI) imageUI->SetEnabled(true);
    onCollision = true;
    GameObject* engineGO = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(otherObject->GetUID());
    
    if (unlockAbility) 
        engineGO->GetComponent<ScriptComponent*>()->GetScriptByType<CuChulainn>()->ActivateAbility(nameAbility);
}
