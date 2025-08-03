#include "pch.h"
#include "Mirage.h"
#include "GameObject.h"
//#include "MeshComponent.h"



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
    state = MirageState::Sleeping;
    stateTimer = 0.0f;
    meshComponent = parent->GetComponent<MeshComponent>();
    return true;
}

void Mirage::Update(float deltaTime)
{
    switch (state)
    {
    case MirageState::Sleeping:
    {
        state      = MirageState::Warning;
        stateTimer = 0.0f;
        GLOG("Calling gameobject");
        /* switch
            to damage visual
        if (meshComponent && mirageWarningImage != 0)
        {
            meshComponent->AddMaterial(mirageWarningImage, false);
        }
        */
        break;
    }

    case MirageState::Warning:
    {
        stateTimer += deltaTime;
        GLOG("Activating gameobject");
        if (stateTimer >= warningDelay)
        {
            state      = MirageState::Damaging;
            stateTimer = 0.0f;
        }
        break;
    }

    case MirageState::Damaging:
    {
        stateTimer += deltaTime;
        GLOG("DISABLING");
        if (stateTimer >= damageDuration)
        {

            state = MirageState::Sleeping;
            parent->SetEnabled(false);
        }
        break;
    }
    }


}
