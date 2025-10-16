

#include "pch.h"

#include "NameDisplay.h"
#include "GameObject.h"

NameDisplay::NameDisplay(GameObject* parent): Script(parent)
{
    fields.emplace_back("Show automatically", InspectorField::FieldType::Bool, &showAutomatically);
    fields.emplace_back("Show delay", InspectorField::FieldType::Float, &showDelay, 0, 10);
    fields.emplace_back("Show duration", InspectorField::FieldType::Float, &showDuration, 1, 10);
}

bool NameDisplay::Init()
{
    if (parent->GetChildren().empty())
    {
        isSetupCorrectly = false;
        GLOG("Name display has no children to show")
        return false;
    }

    for (UID childUID: parent->GetChildren())
        AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(childUID)->SetEnabled(false);
    
    if (showAutomatically) showCounter = showDelay;
    
    return true;
}

void NameDisplay::Update(float deltaTime)
{
    if (!isSetupCorrectly || showed) return;

    if (showCounter <= 0.0f)
    {
        if (childrenVisible)
        {
            for (UID childUID: parent->GetChildren())
                AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(childUID)->SetEnabled(false);
            showed = true;
        } else
            Show();
    }

    showCounter -= deltaTime;
}

void NameDisplay::Show()
{
    if (isSetupCorrectly)
    {
        for (UID childUID: parent->GetChildren())
            AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(childUID)->SetEnabled(true);
        showed = false;
        showCounter = showDuration;
        childrenVisible = true;
    }
}

