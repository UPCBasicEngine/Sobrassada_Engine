#include "pch.h"

#include "Application.h"
#include "MagicBarrier.h"

#include "Character.h"
#include "GameObject.h"
#include "SceneModule.h"
#include "ScriptComponent.h"

MagicBarrier::MagicBarrier(GameObject* parent) : Script(parent)
{
    fields.emplace_back("Area tag", InspectorField::FieldType::InputText, &areaTagString);
}

bool MagicBarrier::Init()
{
    areaTag = HashString(areaTagString);

    GLOG("%s", areaTagString.c_str())
    if (const auto taggedGameObjects = AppEngine->GetSceneModule()->GetScene()->GetTaggedGameObjects(areaTag))
    {
        GLOG("Size: %d", taggedGameObjects->size())
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
    }
}
