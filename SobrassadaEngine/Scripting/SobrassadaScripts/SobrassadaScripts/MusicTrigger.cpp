
#include "pch.h"

#include "MusicTrigger.h"

#include "CuChulainn.h"
#include "GameObject.h"
#include "MusicManager.h"
#include "ScriptComponent.h"
#include "Wwise_IDs.h"
#include "Standalone/Audio/AudioSourceComponent.h"

MusicTrigger::MusicTrigger(GameObject* parent) : Script(parent)
{
    fields.emplace_back("Level state audio event", InspectorField::FieldType::Audio, &levelStateAudioEvent);
    fields.emplace_back("Game state audio event", InspectorField::FieldType::Audio, &gameStateAudioEvent);
    fields.emplace_back("Additional audio event", InspectorField::FieldType::Audio, &additionalAudioEvent);
}

bool MusicTrigger::Init()
{

    GameObject* parentGO = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(parent->GetParent());
    if (parentGO != nullptr && parentGO->GetComponent<ScriptComponent*>() != nullptr)
    {
        cachedMusicManager = parentGO->GetComponent<ScriptComponent*>()->GetScriptByType<MusicManager>();
    }
    if (cachedMusicManager == nullptr)
    {
        isSetupCorrectly = false;
        parent->SetEnabled(false);
        GLOG("[ERROR] Script parents parent does not contain a music manager script")
        return false;
    }
    
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

void MusicTrigger::OnDestroy()
{
    if (audioComp != nullptr) audioComp->EmitEvent(AK::EVENTS::STOP_BACKGROUND_MUSIC);
    Script::OnDestroy();
}

void MusicTrigger::OnCollisionEnter(GameObject* otherObject, const float3 collisionNormal, ColliderLayer layer)
{
    if (otherObject->GetComponent<ScriptComponent*>() != nullptr &&
        otherObject->GetComponent<ScriptComponent*>()->GetScriptByType<CuChulainn>() != nullptr)
    {
        if (levelStateAudioEvent != 0) audioComp->EmitEvent(levelStateAudioEvent);
        if (gameStateAudioEvent != 0)
        {
            audioComp->EmitEvent(gameStateAudioEvent);
            cachedMusicManager->SetCachedGameStateID(gameStateAudioEvent);
        }
        if (additionalAudioEvent != 0) audioComp->EmitEvent(additionalAudioEvent);

        parent->SetEnabled(false);
    }
}
