#pragma once

#include "Module.h"

#include "AkFilePackageLowLevelIODeferred.h"
#include <vector>

#include "HashString.h"
#include <unordered_map>

class AudioSourceComponent;
class AudioListenerComponent;

class AudioModule : public Module
{
  public:
    AudioModule();
    ~AudioModule() override;

    bool Init() override;
    update_status Update(float deltaTime) override;
    bool ShutDown() override;

    void InitAudio();
    void UnloadBanks();
    void musicvol(int volume) { music = volume; };
    void soundvol(int volume) { sound = volume; };
    void voicevol(int volume) { voice = volume; };

    void AddAudioSource(AudioSourceComponent* newSource);
    void RemoveAudioSource(AudioSourceComponent* newSource);
    bool AddAudioListener(AudioListenerComponent* newListener);
    void RemoveAudioListener(AudioListenerComponent* newListener);
    void EmitEvent(const std::string& eventName, AkGameObjectID gameObjectID = 0);
    void RegisterEventName(const std::string& name);

    void StopAllAudio();
    void PlayOnStart();

    const std::unordered_map<HashString, uint32_t>& GetEventsMap() const { return eventsMap; }
    const std::vector<std::string>& GetEventNames();

  private:
    void ParseEvents();

    CAkFilePackageLowLevelIODeferred g_lowLevelIO;
    int music = 0;
    int sound = 0;
    int voice = 0;
    int g_envMAP[255];

    bool loadedAudio         = false;
    bool isPlaying           = false;
    float maxAudibleDistance = 30.0f;

    std::vector<AudioSourceComponent*> sources;
    AudioListenerComponent* listener;
    std::unordered_map<HashString, uint32_t> eventsMap;
    std::vector<std::string> eventNames;
};
