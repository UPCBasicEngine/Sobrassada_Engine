#include "VolumetricAreaComponent.h"

VolumetricAreaComponent::VolumetricAreaComponent(UID uid, GameObject* parent)
    : Component(uid, parent, "Volumetric area", COMPONENT_VOLUMETRIC_AREA)
{
}

VolumetricAreaComponent::VolumetricAreaComponent(const rapidjson::Value& initialState, GameObject* parent)
    : Component(initialState, parent)
{
}

VolumetricAreaComponent::~VolumetricAreaComponent()
{
}

void VolumetricAreaComponent::Init()
{
}

void VolumetricAreaComponent::Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const
{
    Component::Save(targetState, allocator);
}

void VolumetricAreaComponent::Clone(const Component* other)
{
}

void VolumetricAreaComponent::RenderEditorInspector()
{
}

void VolumetricAreaComponent::Update(float deltaTime)
{
}

void VolumetricAreaComponent::RenderDebug(float deltaTime)
{
}
