#include "pch.h"
#include "Mirage.h"



Mirage::Mirage(GameObject* parent) : Script(parent)
{
    //fields.push_back({"Shape", InspectorField::FieldType::Enum, &ShapeType, 0, 1});
    fields.push_back({"Delay Before Damage", InspectorField::FieldType::Float, &warningDelay, 0.0f, 10.0f});
    fields.push_back({"Damage Duration", InspectorField::FieldType::Float, &damageDuration, 0.0f, 10.0f});
    fields.push_back({"Damage", InspectorField::FieldType::Int, &damage, 0, 100});
    fields.push_back({"Size X/Radius", InspectorField::FieldType::Float, &sizeX, 0.1f, 50.0f});
    fields.push_back({"Size Y", InspectorField::FieldType::Float, &sizeY, 0.1f, 50.0f});
    fields.push_back({"Rotation Y", InspectorField::FieldType::Float, &rotation, -180.0f, 180.0f});
}


void Mirage::Update(float deltaTime)
{

}
