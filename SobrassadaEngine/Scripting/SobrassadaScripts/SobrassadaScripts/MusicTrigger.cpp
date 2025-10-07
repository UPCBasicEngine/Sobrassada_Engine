
#include "pch.h"

#include "MusicTrigger.h"

#include "CuChulainn.h"
#include "GameObject.h"
#include "ScriptComponent.h"
#include "Wwise_IDs.h"
#include "Standalone/Audio/AudioSourceComponent.h"

MusicTrigger::MusicTrigger(GameObject* parent): Script(parent)
{
    fields.emplace_back("Audio to emit", InspectorField::FieldType::Audio, &audioToEmit);
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
        audioComp->EmitEvent(audioToEmit);
        parent->SetEnabled(false);
    }
}

