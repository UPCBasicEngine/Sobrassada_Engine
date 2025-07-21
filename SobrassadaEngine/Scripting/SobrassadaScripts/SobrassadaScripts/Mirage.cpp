#include "pch.h"
#include "Mirage.h"
#include "GameObject.h"

#include "Math/Quat.h"
#include "Math/float4x4.h"

Mirage::Mirage(GameObject* parent) : Script(parent)
{

    fields.push_back({"Delay Before Damage", InspectorField::FieldType::Float, &warningDelay, 0.0f, 10.0f});
    fields.push_back({"Damage Duration", InspectorField::FieldType::Float, &damageDuration, 0.0f, 10.0f});
    fields.push_back({"Damage", InspectorField::FieldType::Int, &damage, 0, 100});
    fields.push_back({"Weight Order", InspectorField::FieldType::Int, &weightOrder, 0, 100});
}

bool Mirage::Init()
{
    state = MirageState::Sleeping;

    parent->SetEnabled(false);
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

        // switch to damage visual

        break;
    }

    case MirageState::Warning:
    {
        stateTimer += deltaTime;
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
        if (stateTimer >= damageDuration)
        {

            state = MirageState::Sleeping;
            parent->SetEnabled(false);
            break;
        }
    }
    }
}
