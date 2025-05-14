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


