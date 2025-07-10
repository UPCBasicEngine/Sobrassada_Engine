#include "pch.h"
#include "Mirage.h"

#include "GameObject.h"

#include "Math/float4x4.h"
#include "Math/Quat.h"

Mirage::Mirage(GameObject* parent) : Script(parent)
{

    fields.push_back({"Delay Before Damage", InspectorField::FieldType::Float, &warningDelay, 0.0f, 10.0f});
    fields.push_back({"Damage Duration", InspectorField::FieldType::Float, &damageDuration, 0.0f, 10.0f});
    fields.push_back({"Damage", InspectorField::FieldType::Int, &damage, 0, 100});
    fields.push_back(
        {"Set Mirage",
         [this](Script* self)
         {
             const float4x4& currentTransform = this->parent->GetLocalTransform();

             this->startPosition              = currentTransform.TranslatePart();
             this->startRotation              = currentTransform.RotatePart().ToEulerXYZ();
             this->startScale                 = currentTransform.GetScale();
             Quat rotQuat                     = Quat(currentTransform.RotatePart());
         }}
    );
}


void Mirage::Update(float deltaTime)
{

}
