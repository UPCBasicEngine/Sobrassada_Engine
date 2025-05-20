#pragma once

#include "Component.h"
#include <AK/SoundEngine/Common/AkTypes.h>

class SOBRASADA_API_ENGINE AudioSourceComponent : public Component
{
  public:
    AudioSourceComponent(UID uid, GameObject* parent);
    AudioSourceComponent(const rapidjson::Value& initialState, GameObject* parent);
    ~AudioSourceComponent() override;

    void Init() override;
    void Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const override;
    void Clone(const Component* other) override;

    void Update(float deltaTime) override;
    void Render(float deltaTime) override {};
    void RenderEditorInspector() override;
    virtual void RenderDebug(float deltaTime) {};

    // More efficient to use the IDs, but both exist in case it is needed to use the string variant in some scenario
    void EmitDefaultEvent();
    void EmitEvent(const AkUniqueID event);
    void EmitEvent(const std::string& event);
    void SetRTPCValue(const AkUniqueID parameterID, const float value);
    void SetRTPCValue(const std::string& parameterName, const float value);
    void SetSwitch(const AkUniqueID switchGroupID, const AkUniqueID activeSwitchID);
    void SetSwitch(const std::string& switchGroupName, const std::string& activeSwitchName);

    void SetDefaultEvent(const AkUniqueID newEvent);
    void SetVolume(const float newVolume);
    void SetPitch(const float newPitch);
    void SetSpatialization(const float newSpatialization);

    bool IsPlayOnStart() const { return playOnStart; }
    void UpdateEventsNames();
    void StopAudio() const;
    void StopAllAudio() const;

  private:
    void SetInitValues();

    std::string defaultEventName = "Default";
    AkUniqueID defaultEvent;
    AkPlayingID playingEvent;

    float volume         = 1;
    float pitch          = 0.5f;
    float spatialization = 0;

    bool isInited        = false;
    bool playOnStart     = false;
};
