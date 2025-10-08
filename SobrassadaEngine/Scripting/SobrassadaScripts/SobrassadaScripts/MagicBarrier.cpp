#include "pch.h"

#include "Application.h"
#include "MagicBarrier.h"

#include "Character.h"
#include "GameObject.h"
#include "SceneModule.h"
#include "ScriptComponent.h"
#include "Standalone/Audio/AudioSourceComponent.h"

MagicBarrier::MagicBarrier(GameObject* parent) : Script(parent)
{
    fields.emplace_back("Area tag", InspectorField::FieldType::InputText, &areaTagString);
    fields.emplace_back("Removal audio event", InspectorField::FieldType::Audio, &onRemovalAudioEvent);
}

bool MagicBarrier::Init()
{
    // Audio
    audioComp = parent->GetComponent<AudioSourceComponent*>();
    if (audioComp == nullptr)
    {
        parent->SetEnabled(false);
        GLOG("[ERROR] Script parent does not contain an audio component")
        return false;
    }
    
    enemiesInArea = 0;
    areaTag = HashString(areaTagString);

    if (const auto taggedGameObjects = AppEngine->GetSceneModule()->GetScene()->GetTaggedGameObjects(areaTag))
    {
        for (GameObject* currentGameObject : *taggedGameObjects)
        {
            if (ScriptComponent* script = currentGameObject->GetComponent<ScriptComponent*>())
            {
                if (Character* character = script->GetScriptByType<Character>())
                {
                    character->SetAssociatedBarrier(this);
                    enemiesInArea++;
                }
            }
        }
    }

    return true;
}

void MagicBarrier::EnemyDied()
{
    enemiesInArea--;
    if (enemiesInArea <= 0)
    {
        parent->SetEnabledRecursive(false);
        if (audioComp != nullptr && onRemovalAudioEvent != 0) audioComp->EmitEvent(onRemovalAudioEvent);
    }
}
