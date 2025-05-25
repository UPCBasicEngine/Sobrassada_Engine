#include "TrailComponent.h"

TrailComponent::TrailComponent(UID uid, GameObject* parent) : Component(uid, parent, "Trail", COMPONENT_TRAIL)
{
}

TrailComponent::TrailComponent(const rapidjson::Value& initialState, GameObject* parent)
    : Component(initialState, parent)
{
}

TrailComponent::~TrailComponent()
{
}

void TrailComponent::Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const
{
}

void TrailComponent::Clone(const Component* other)
{
}

void TrailComponent::Update(float deltaTime)
{
}

void TrailComponent::Render(float deltaTime)
{
}

void TrailComponent::RenderDebug(float deltaTime)
{
}

void TrailComponent::RenderEditorInspector()
{
}

void TrailComponent::ParentUpdated()
{
}
