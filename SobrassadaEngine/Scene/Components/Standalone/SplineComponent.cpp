#include "SplineComponent.h"

#include "GameObject.h"

SplineComponent::SplineComponent(UID uid, GameObject* parent) : Component(uid, parent, "Spline", COMPONENT_SPLINE)
{
}

SplineComponent::SplineComponent(const rapidjson::Value& initialState, GameObject* parent)
    : Component(initialState, parent)
{
}


SplineComponent::~SplineComponent()
{
}

void SplineComponent::Update(float deltaTime)
{
}

void SplineComponent::Render(float deltaTime)
{
}

void SplineComponent::RenderDebug(float deltaTime)
{
}

void SplineComponent::RenderEditorInspector()
{
}

void SplineComponent::Save(rapidjson::Value&, rapidjson::Document::AllocatorType&) const
{
}

void SplineComponent::Clone(const Component* other)
{
}

void SplineComponent::AddPoint(const float3& p)
{
    points.push_back(p);
}

void SplineComponent::InsertPoint(size_t i, const float3& p)
{
    points.insert(points.begin() + i, p);
}

void SplineComponent::RemovePoint(size_t i)
{
    if (i < points.size()) points.erase(points.begin() + i);
}




