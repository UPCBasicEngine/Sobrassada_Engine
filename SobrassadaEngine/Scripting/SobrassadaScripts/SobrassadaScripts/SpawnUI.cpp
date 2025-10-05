#include "pch.h"

#include "GameObject.h"
#include "SpawnUI.h"
#include "Standalone/Physics/SphereColliderComponent.h"
#include "Standalone/UI/ImageComponent.h"
#include "ScriptComponent.h"
#include "CuChulainn.h"
#include "InputModule.h"

SpawnUI::SpawnUI(GameObject* parent) : Script(parent)
{
    fields.push_back({"Object UI Name", InspectorField::FieldType::InputText, &objectUIName});
    fields.push_back({"Alternative object UI Name", InspectorField::FieldType::InputText, &alternativeObjectUIName});
    fields.push_back({"Trigger once", InspectorField::FieldType::Bool, &triggerOnce});
    fields.push_back({"Hide ui after (s) (0 = don´t hide)", InspectorField::FieldType::Float, &hideAfterSeconds, 0.f, 30.f});
    
    fields.push_back({"Unlock Ability", InspectorField::FieldType::Bool, &unlockAbility});
    fields.push_back({"Ability Name", InspectorField::FieldType::InputText, &nameAbility});
}

bool SpawnUI::Init()
{
    trigger = parent->GetComponent<SphereColliderComponent*>();
    if (!trigger) GLOG("[WARNING] SpawnUI without sphere collider component.");

    if (const GameObject* objectUI = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(objectUIName))
    {
        imageUI = objectUI->GetComponent<ImageComponent*>();
        if (imageUI) imageUI->SetEnabled(false);
        else GLOG("[WARNING] No Image component in game object: %s", objectUIName.c_str());
    }
    else GLOG("[WARNING] No UI game object found by the name: %s", objectUIName.c_str());

    if (const GameObject* alternativeObjectUI = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(alternativeObjectUIName))
    {
        alternativeImageUI = alternativeObjectUI->GetComponent<ImageComponent*>();
        if (alternativeImageUI) alternativeImageUI->SetEnabled(false);
    }

    return true;
}

void SpawnUI::Update(float deltaTime)
{
    if (!updating || !trigger || !imageUI) return;
    if (hideAfterSeconds > 0.f)
    {
        timer -= deltaTime;
        if (timer <= 0.f)
        {
            if (imageUI != nullptr && !unlockAbility) imageUI->SetEnabled(false);
            if (alternativeImageUI != nullptr) alternativeImageUI->SetEnabled(false);
            updating = false;
        }
    }
}

void SpawnUI::OnCollisionEnter(GameObject* otherObject, const float3 collisionNormal, ColliderLayer layer)
{
    // triggers only collision with Player
    
    if (imageUI != nullptr)
    {
        if (alternativeImageUI != nullptr && AppEngine->GetInputModule()->IsControllerConnected())
            alternativeImageUI->SetEnabled(true);
        else 
            imageUI->SetEnabled(true);
    }

    timer = hideAfterSeconds;
    
    if (unlockAbility) 
        otherObject->GetComponent<ScriptComponent*>()->GetScriptByType<CuChulainn>()->ActivateAbility(nameAbility);

    updating = true;
}

void SpawnUI::OnCollisionExit(GameObject* otherObject, ColliderLayer layer)
{
    if (hideAfterSeconds == 0 && imageUI != nullptr) imageUI->SetEnabled(false);
    if (hideAfterSeconds == 0 && alternativeImageUI != nullptr) alternativeImageUI->SetEnabled(false);
    if (triggerOnce) trigger->SetEnabled(false);

    if (hideAfterSeconds == 0) updating = false;
}

