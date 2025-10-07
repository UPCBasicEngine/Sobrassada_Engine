
#include "pch.h"
#include "MusicManager.h"
#include "GameObject.h"
#include "Standalone/Audio/AudioSourceComponent.h"

MusicManager::MusicManager(GameObject* parent): Script(parent)
{
    fields.emplace_back("First audio event", InspectorField::FieldType::Audio, &firstRespawnAudioEvent);
    fields.emplace_back("Second audio event", InspectorField::FieldType::Audio, &secondRespawnAudioEvent);
    fields.emplace_back("Third audio event", InspectorField::FieldType::Audio, &thirdRespawnAudioEvent);
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
    return true;
}

void MusicManager::OnPlayerRespawn() const
{
    for (UID childUID: parent->GetChildren())
    {
        AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(childUID)->SetEnabled(true);

        if (firstRespawnAudioEvent != 0) audioComp->EmitEvent(firstRespawnAudioEvent);
        if (secondRespawnAudioEvent != 0) audioComp->EmitEvent(secondRespawnAudioEvent);
        if (thirdRespawnAudioEvent != 0) audioComp->EmitEvent(thirdRespawnAudioEvent);
    }
}


