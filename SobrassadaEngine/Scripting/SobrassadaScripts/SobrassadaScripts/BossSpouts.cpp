#include "pch.h"
#include "BossSpouts.h"
#include "GameObject.h"
#include "Math/Quat.h"

BossSpouts::BossSpouts(GameObject* parent) : Script(parent)
{
    fields.push_back({"Center", InspectorField::FieldType::Vec3, &center});
    fields.push_back({"Radius", InspectorField::FieldType::Float, &radius, 0.0f, 50.0f});
    fields.push_back({"Speed", InspectorField::FieldType::Float, &speed, -10.0f, 10.0f});
    fields.push_back({"VerticalOffset", InspectorField::FieldType::Float, &verticalOffset, -10.0f, 10.0f});
}

bool BossSpouts::Init()
{
    angle = 0.0f;
    return true;
}

void BossSpouts::Update(float deltaTime)
{
    angle   += speed * deltaTime;

    float x  = center.x + cosf(angle) * radius;
    float z  = center.z + sinf(angle) * radius;
    float y  = center.y + verticalOffset;

    parent->SetLocalPosition(float3(x, y, z));
}