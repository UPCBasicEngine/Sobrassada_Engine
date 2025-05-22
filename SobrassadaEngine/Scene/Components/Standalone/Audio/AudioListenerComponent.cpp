#include "AudioListenerComponent.h"

#include "Application.h"
#include "AudioModule.h"

#include "ImGui.h"

AudioListenerComponent::AudioListenerComponent(UID uid, GameObject* parent)
    : Component(uid, parent, "Audio Listener", COMPONENT_AUDIO_LISTENER)
{
}

AudioListenerComponent::AudioListenerComponent(const rapidjson::Value& initialState, GameObject* parent)
    : Component(initialState, parent)
{
}

AudioListenerComponent::~AudioListenerComponent()
{
    App->GetAudioModule()->RemoveAudioListener(this);
}

void AudioListenerComponent::Init()
{
    if (App->GetAudioModule()->AddAudioListener(this)) isActiveListener = true;
}

void AudioListenerComponent::Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const
{
    Component::Save(targetState, allocator);
}

void AudioListenerComponent::Clone(const Component* other)
{
    if (other->GetType() == ComponentType::COMPONENT_AUDIO_LISTENER)
    {
        const AudioListenerComponent* otherAudioListener = static_cast<const AudioListenerComponent*>(other);
        enabled                                          = otherAudioListener->enabled;
        wasEnabled                                       = otherAudioListener->wasEnabled;
    }
    else
    {
        GLOG("It is not possible to clone a component of a different type!");
    }
}

void AudioListenerComponent::RenderEditorInspector()
{
    Component::RenderEditorInspector();

    if (!isActiveListener)
    {
        ImGui::Text("There is already an active Audio Listener in the scene.\n In order to use a new one, you must "
                    "delete the existing one before!");
        return;
    }
}
