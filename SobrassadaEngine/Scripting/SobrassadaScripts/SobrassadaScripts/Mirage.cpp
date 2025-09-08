#include "pch.h"
#include "Mirage.h"
#include "Application.h"
#include "GameObject.h"
#include "MirageBossDash.h"
#include "SceneModule.h"
#include "ScriptComponent.h"
#include "Standalone\MeshComponent.h"


Mirage::Mirage(GameObject* parent) : Script(parent)
{

    fields.push_back({"Delay Before Damage", InspectorField::FieldType::Float, &warningDelay, 0.0f, 10.0f});
    fields.push_back({"Damage Duration", InspectorField::FieldType::Float, &damageDuration, 0.0f, 10.0f});
    fields.push_back({"Damage", InspectorField::FieldType::Int, &damage, 0, 100});
    fields.push_back({"Weight Order", InspectorField::FieldType::Int, &weightOrder, 0, 100});
    fields.push_back({"Mirage Warning", InspectorField::FieldType::Resource, &mirageWarningImage});
    fields.push_back({"Mirage Damage", InspectorField::FieldType::Resource, &mirageDamageImage});
}

bool Mirage::Init()
{
    Scene* scene = AppEngine->GetSceneModule()->GetScene();
    //GLOG("MIRAGE DETECTED");
    state                       = MirageState::Sleeping;
    stateTimer                  = 0.0f;
    meshComponent               = parent->GetComponent<MeshComponent*>();
    std::vector<UID> children   = parent->GetChildren();

    GameObject* firstChild      = scene->GetGameObjectByUID(children[0]);

    ScriptComponent* scriptComp = firstChild->GetComponent<ScriptComponent*>();

    GameObject* secondChild     = scene->GetGameObjectByUID(children[1]);

    bossDash                    = scriptComp->GetScriptByType<MirageBossDash>();
    endPoint                    = secondChild->GetLocalTransform().TranslatePart();
    //GLOG("Mirage endpoint at %f,%f,%f", endPoint.x, endPoint.y, endPoint.z);
    bossDash->setEndPoint(endPoint);

    parent->SetEnabled(false);

    return true;
}

void Mirage::Update(float deltaTime)
{
    switch (state)
    {
    case MirageState::Sleeping:
    {
        parent->SetEnabled(true);
        state      = MirageState::Warning;
        stateTimer = 0.0f;
        GLOG("Calling gameobject");

        if (meshComponent && mirageWarningImage != 0)
        {
            // meshComponent->AddMaterial(mirageWarningImage, false);
        }

        break;
    }

    case MirageState::Warning:
    {
        stateTimer += deltaTime;
        GLOG("Activating gameobject");

        if (meshComponent && mirageWarningImage != 0)
        {
            // meshComponent->AddMaterial(mirageDamageImage, false);
        }

        if (stateTimer >= warningDelay)
        {
            state      = MirageState::Damaging;
            stateTimer = 0.0f;

            if (bossDash)
            {
                bossDash->setState(BossDashStates::OverheadStrike);
                bossDash->setAction(BossDashActions::Prepare);
                bossDash->setStateBool(true);
            }
        }
        break;
    }

    case MirageState::Damaging:
    {
        stateTimer += deltaTime;
        if (stateTimer >= damageDuration)
        {

            state = MirageState::Sleeping;
            parent->SetEnabled(false);
        }
        break;
    }
    }
}