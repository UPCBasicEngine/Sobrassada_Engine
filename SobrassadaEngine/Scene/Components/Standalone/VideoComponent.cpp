#include "VideoComponent.h"

VideoComponent::VideoComponent(UID uid, GameObject* parent) : Component(uid, parent, "Video", COMPONENT_VIDEO)
{
}

VideoComponent::VideoComponent(const rapidjson::Value& initialState, GameObject* parent)
    : Component(initialState, parent)
{
}

VideoComponent::~VideoComponent()
{
}

void VideoComponent::Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const
{
    Component::Save(targetState, allocator);
}

void VideoComponent::Clone(const Component* other)
{
    if (other->GetType() == ComponentType::COMPONENT_VIDEO)
    {
        const VideoComponent* videoComponent = static_cast<const VideoComponent*>(other);
        
    }
}

void VideoComponent::Update(float deltaTime)
{
}

void VideoComponent::Render(float deltaTime)
{
}

void VideoComponent::RenderDebug(float deltaTime)
{
}

void VideoComponent::RenderEditorInspector()
{
}

void VideoComponent::ParentUpdated()
{
}
