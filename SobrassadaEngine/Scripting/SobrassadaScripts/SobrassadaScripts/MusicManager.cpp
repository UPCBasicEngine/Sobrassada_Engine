
#include "pch.h"

#include "GameObject.h"
#include "MusicManager.h"
#include "Standalone/Audio/AudioSourceComponent.h"
#include "Wwise_IDs.h"

#include <AK/SoundEngine/Common/AkSoundEngine.h>

MusicManager::MusicManager(GameObject* parent) : Script(parent)
{
    fields.emplace_back("First audio event", InspectorField::FieldType::Audio, &levelStateAudioEvent);
    fields.emplace_back("Second audio event", InspectorField::FieldType::Audio, &gameStateAudioEvent);
    fields.emplace_back("Third audio event", InspectorField::FieldType::Audio, &additionalAudioEvent);
}

bool MusicManager::Init()
{
    // Audio
    audioComp = parent->GetComponent<AudioSourceComponent*>();
    if (audioComp == nullptr)
    {
        isSetupCorrectly = false;
        parent->SetEnabled(false);
        GLOG("[ERROR] Script parent does not contain an audio component")
        return false;
    }

    if (levelStateAudioEvent != 0) audioComp->EmitEvent(levelStateAudioEvent);
    if (gameStateAudioEvent != 0)
    {
        audioComp->EmitEvent(gameStateAudioEvent);
        cachedGameStateID = gameStateAudioEvent;
    }
    if (additionalAudioEvent != 0) audioComp->EmitEvent(additionalAudioEvent);

    return true;
}

void MusicManager::OnDestroy()
{
    if (audioComp != nullptr) audioComp->EmitEvent(AK::EVENTS::STOP_BACKGROUND_MUSIC);
    Script::OnDestroy();
}

void MusicManager::OnPlayerRespawn()
{
    for (UID childUID : parent->GetChildren())
    {
        AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(childUID)->SetEnabled(true);

        if (levelStateAudioEvent != 0) audioComp->EmitEvent(levelStateAudioEvent);
        if (gameStateAudioEvent != 0)
        {
            audioComp->EmitEvent(gameStateAudioEvent);
            cachedGameStateID = gameStateAudioEvent;
        }
    }
}

void MusicManager::ResetToCachedGameState() const
{
    if (cachedGameStateID != 0) audioComp->EmitEvent(cachedGameStateID);
}
