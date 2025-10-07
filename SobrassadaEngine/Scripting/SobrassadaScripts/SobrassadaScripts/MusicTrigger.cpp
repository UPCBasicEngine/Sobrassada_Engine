
#include "pch.h"

#include "MusicTrigger.h"

#include "CuChulainn.h"
#include "GameObject.h"
#include "ScriptComponent.h"
#include "Standalone/Audio/AudioSourceComponent.h"

MusicTrigger::MusicTrigger(GameObject* parent) : Script(parent)
{
    fields.emplace_back("First audio event", InspectorField::FieldType::Audio, &firstAudioEvent);
    fields.emplace_back("Second audio event", InspectorField::FieldType::Audio, &secondAudioEvent);
    fields.emplace_back("Third audio event", InspectorField::FieldType::Audio, &thirdAudioEvent);
}

bool MusicTrigger::Init()
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

void MusicTrigger::OnCollisionEnter(GameObject* otherObject, const float3 collisionNormal, ColliderLayer layer)
{
    if (otherObject->GetComponent<ScriptComponent*>() != nullptr &&
        otherObject->GetComponent<ScriptComponent*>()->GetScriptByType<CuChulainn>() != nullptr)
    {
        if (firstAudioEvent != 0) audioComp->EmitEvent(firstAudioEvent);
        if (secondAudioEvent != 0) audioComp->EmitEvent(secondAudioEvent);
        if (thirdAudioEvent != 0) audioComp->EmitEvent(thirdAudioEvent);

        parent->SetEnabled(false);
    }
}
