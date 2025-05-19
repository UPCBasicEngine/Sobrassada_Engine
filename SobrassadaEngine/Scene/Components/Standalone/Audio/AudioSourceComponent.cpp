#include "AudioSourceComponent.h"

#include "Application.h"
#include "AudioModule.h"
#include "EditorUIModule.h"
#include "GameObject.h"
#include "InputModule.h" // TODO:  Delete this after testing

#include "AK/SoundEngine/Common/AkSoundEngine.h"
#include "ImGui.h"

AudioSourceComponent::AudioSourceComponent(UID uid, GameObject* parent)
    : Component(uid, parent, "Audio Source", COMPONENT_AUDIO_SOURCE)
{
}

AudioSourceComponent::AudioSourceComponent(const rapidjson::Value& initialState, GameObject* parent)
    : Component(initialState, parent)
{
    defaultEvent   = initialState["DefaultEvent"].GetUint();
    volume         = initialState["Volume"].GetFloat();
    pitch          = initialState["Pitch"].GetFloat();
    spatialization = initialState["Spatialization"].GetFloat();
}

AudioSourceComponent::~AudioSourceComponent()
{
    App->GetAudioModule()->RemoveAudioSource(this);
}

void AudioSourceComponent::Init()
{
    App->GetAudioModule()->AddAudioSource(this);
    SetInitValues();
    isInited = true;
}

void AudioSourceComponent::Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const
{
    Component::Save(targetState, allocator);

    targetState.AddMember("DefaultEvent", defaultEvent, allocator);
    targetState.AddMember("Volume", volume, allocator);
    targetState.AddMember("Pitch", pitch, allocator);
    targetState.AddMember("Spatialization", spatialization, allocator);
}

void AudioSourceComponent::Clone(const Component* other)
{
    if (other->GetType() == ComponentType::COMPONENT_AUDIO_SOURCE)
    {
        const AudioSourceComponent* otherAudioSource = static_cast<const AudioSourceComponent*>(other);
        enabled                                      = otherAudioSource->enabled;
        wasEnabled                                   = otherAudioSource->wasEnabled;

        defaultEvent                                 = otherAudioSource->defaultEvent;
        volume                                       = otherAudioSource->volume;
        pitch                                        = otherAudioSource->pitch;
        spatialization                               = otherAudioSource->spatialization;

        SetInitValues();
        UpdateEventsNames();
    }
    else
    {
        GLOG("It is not possible to clone a component of a different type!");
    }
}

void AudioSourceComponent::Update(float deltaTime)
{
    if (App->GetInputModule()->GetKeyboard()[SDL_SCANCODE_0] == KEY_DOWN)
    {
        //GLOG("Play audio");
        EmitEvent(defaultEvent);
    }
}

void AudioSourceComponent::RenderEditorInspector()
{
    Component::RenderEditorInspector();

    ImGui::SeparatorText("Audio Soure");
    ImGui::Text(defaultEventName.c_str());
    ImGui::SameLine();
    if (ImGui::Button("Select default event"))
    {
        ImGui::OpenPopup(CONSTANT_EVENT_SELECT_DIALOG_ID);
    }

    if (ImGui::IsPopupOpen(CONSTANT_EVENT_SELECT_DIALOG_ID))
    {
        SetDefaultEvent(App->GetEditorUIModule()->RenderResourceSelectDialog<uint32_t>(
            CONSTANT_EVENT_SELECT_DIALOG_ID, App->GetAudioModule()->GetEventsMap(), (uint32_t)0
        ));
    }

    if (ImGui::DragFloat("Volume", &volume, 0.01f, 0, 1, "%.3f", ImGuiSliderFlags_AlwaysClamp)) SetVolume(volume);
    if (ImGui::DragFloat("Pitch", &pitch, 0.01f, 0, 1, "%.3f", ImGuiSliderFlags_AlwaysClamp)) SetPitch(pitch);
    if (ImGui::DragFloat("3D Spatialization", &spatialization, 0.01f, 0, 1, "%.3f", ImGuiSliderFlags_AlwaysClamp))
        SetSpatialization(spatialization);
}

void AudioSourceComponent::EmitEvent(const AkUniqueID event)
{
    if (enabled) playingEvent = AK::SoundEngine::PostEvent(event, (AkGameObjectID)parent->GetUID());
}

void AudioSourceComponent::EmitEvent(const std::string& event)
{
    if (enabled) playingEvent = AK::SoundEngine::PostEvent(event.c_str(), (AkGameObjectID)parent->GetUID());
}

void AudioSourceComponent::SetVolume(const float newVolume)
{
    volume = newVolume;
    AK::SoundEngine::SetRTPCValue("Volume", volume, parent->GetUID());
}

void AudioSourceComponent::SetPitch(const float newPitch)
{
    pitch = newPitch;
    AK::SoundEngine::SetRTPCValue("Pitch", pitch, parent->GetUID());
}

void AudioSourceComponent::SetSpatialization(const float newSpatialization)
{
    spatialization = newSpatialization;
    AK::SoundEngine::SetRTPCValue("Spatialization", spatialization, parent->GetUID());
}

void AudioSourceComponent::SetInitValues()
{
    AK::SoundEngine::SetRTPCValue("Volume", volume, parent->GetUID());
    AK::SoundEngine::SetRTPCValue("Pitch", pitch, parent->GetUID());
    AK::SoundEngine::SetRTPCValue("Spatialization", spatialization, parent->GetUID());
}

void AudioSourceComponent::SetRTPCValue(const AkUniqueID parameterID, const float value)
{
    AK::SoundEngine::SetRTPCValue(parameterID, value, parent->GetUID());
}

void AudioSourceComponent::SetRTPCValue(const std::string& parameterName, const float value)
{
    AK::SoundEngine::SetRTPCValue(parameterName.c_str(), value, parent->GetUID());
}

void AudioSourceComponent::SetSwitch(const AkUniqueID switchGroupID, const AkUniqueID activeSwitchID)
{
    AK::SoundEngine::SetSwitch(switchGroupID, activeSwitchID, parent->GetUID());
}

void AudioSourceComponent::SetSwitch(const std::string& switchGroupName, const std::string& activeSwitchName)
{
    AK::SoundEngine::SetSwitch(switchGroupName.c_str(), activeSwitchName.c_str(), parent->GetUID());
}

void AudioSourceComponent::SetDefaultEvent(const AkUniqueID newEvent)
{
    if (newEvent == 0) return;

    defaultEvent = newEvent;
    UpdateEventsNames();
}

void AudioSourceComponent::UpdateEventsNames()
{
    for (const auto& event : App->GetAudioModule()->GetEventsMap())
    {
        if (event.second == defaultEvent) defaultEventName = event.first.GetString();
    }
}

void AudioSourceComponent::StopAudio() const
{
    AK::SoundEngine::StopPlayingID(playingEvent);
}

void AudioSourceComponent::StopAllAudio() const
{
    App->GetAudioModule()->StopAllAudio();
}