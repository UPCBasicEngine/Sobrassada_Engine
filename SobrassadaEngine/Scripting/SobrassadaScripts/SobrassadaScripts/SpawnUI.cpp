#include "pch.h"

#include "CuChulainn.h"
#include "GameObject.h"
#include "InputModule.h"
#include "ScriptComponent.h"
#include "SpawnUI.h"
#include "Standalone/Physics/SphereColliderComponent.h"
#include "Standalone/UI/ImageComponent.h"

SpawnUI::SpawnUI(GameObject* parent) : Script(parent)
{
    fields.push_back({"Object UI Name", InspectorField::FieldType::InputText, &objectUIName});
    fields.push_back(
        {"Alternative object UI Name (XBox)", InspectorField::FieldType::InputText, &xboxAlternativeObjectUIName}
    );
    fields.push_back(
        {"Alternative object UI Name (PS)", InspectorField::FieldType::InputText, &psAlternativeObjectUIName}
    );
    fields.push_back({"Trigger once", InspectorField::FieldType::Bool, &triggerOnce});

    fields.push_back({"Show ui after (s)", InspectorField::FieldType::Float, &showDelay, 0.f, 30.f});
    fields.push_back(
        {"Hide ui after (s) (0 = don´t hide)", InspectorField::FieldType::Float, &hideAfterSeconds, 0.f, 30.f}
    );

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

    if (const GameObject* alternativeObjectUI =
            AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(xboxAlternativeObjectUIName))
    {
        xboxAlternativeImageUI = alternativeObjectUI->GetComponent<ImageComponent*>();
        if (xboxAlternativeImageUI) xboxAlternativeImageUI->SetEnabled(false);
    }

    if (const GameObject* alternativeObjectUI =
            AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(psAlternativeObjectUIName))
    {
        psAlternativeImageUI = alternativeObjectUI->GetComponent<ImageComponent*>();
        if (psAlternativeImageUI) psAlternativeImageUI->SetEnabled(false);
    }

    return true;
}

void SpawnUI::Update(float deltaTime)
{
    if (!updating || !trigger || !imageUI) return;

    if (delayedShowing)
    {
        timer -= deltaTime;
        if (timer <= 0.f)
        {
            ShowUI();
        }
        return;
    }

    if (hideAfterSeconds > 0.f)
    {
        timer -= deltaTime;
        if (timer <= 0.f)
        {
            if (imageUI != nullptr && !unlockAbility) imageUI->SetEnabled(false);
            if (xboxAlternativeImageUI != nullptr && !unlockAbility) xboxAlternativeImageUI->SetEnabled(false);
            if (psAlternativeImageUI != nullptr && !unlockAbility) psAlternativeImageUI->SetEnabled(false);
            updating = false;
        }
    }
}

void SpawnUI::OnCollisionEnter(GameObject* otherObject, const float3 collisionNormal, ColliderLayer layer)
{
    // triggers only collision with Player
    cachedCollisionObject = otherObject;
    timer                 = showDelay;
    updating              = true;
    delayedShowing        = true;

    if (showDelay == 0) ShowUI();
}

void SpawnUI::OnCollisionExit(GameObject* otherObject, ColliderLayer layer)
{
    if (hideAfterSeconds == 0 && imageUI != nullptr) imageUI->SetEnabled(false);
    if (hideAfterSeconds == 0 && xboxAlternativeImageUI != nullptr) xboxAlternativeImageUI->SetEnabled(false);
    if (hideAfterSeconds == 0 && psAlternativeImageUI != nullptr) psAlternativeImageUI->SetEnabled(false);
    if (triggerOnce) trigger->SetEnabled(false);

    if (hideAfterSeconds == 0) updating = false;
}

void SpawnUI::ShowUI()
{
    if (imageUI != nullptr)
    {

        if (AppEngine->GetInputModule()->IsControllerConnected())
            if (psAlternativeImageUI != nullptr && AppEngine->GetInputModule()->IsPlaystationControllerConnected())
                psAlternativeImageUI->SetEnabled(true);
            else if (xboxAlternativeImageUI != nullptr) xboxAlternativeImageUI->SetEnabled(true);
            else imageUI->SetEnabled(true);
        else imageUI->SetEnabled(true);
    }

    timer          = hideAfterSeconds;
    delayedShowing = false;

    if (unlockAbility)
        cachedCollisionObject->GetComponent<ScriptComponent*>()->GetScriptByType<CuChulainn>()->ActivateAbility(
            nameAbility
        );
}
