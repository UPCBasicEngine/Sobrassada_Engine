#include "pch.h"

#include "Mirage.h"
#include "GameObject.h"

#include "Math/Quat.h"
#include "Math/float4x4.h"

enum MIRAGE_STATE
{
    SLEEPING,
    WARNING,
    DAMAGING,
};

Mirage::Mirage(GameObject* parent) : Script(parent)
{

    fields.push_back({"Delay Before Damage", InspectorField::FieldType::Float, &warningDelay, 0.0f, 10.0f});
    fields.push_back({"Damage Duration", InspectorField::FieldType::Float, &damageDuration, 0.0f, 10.0f});
    fields.push_back({"Damage", InspectorField::FieldType::Int, &damage, 0, 100});
    fields.push_back({"Weight Order", InspectorField::FieldType::Int, &weightOrder, 0, 100});
    
}

bool Mirage::Init()
{
    return true;
}

void Mirage::Update(float deltaTime)
{
}
