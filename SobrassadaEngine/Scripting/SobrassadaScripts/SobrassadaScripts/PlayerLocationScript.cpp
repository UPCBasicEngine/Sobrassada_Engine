#include "pch.h"
#include "PlayerLocationScript.h"


#include "Scene.h"
#include "GameObject.h"
#include "SceneModule.h"
#include "ScriptComponent.h"
#include "Character.h"

PlayerLocationScript::PlayerLocationScript(GameObject* parent) : Script(parent)
{
    fields.push_back({"Location tag", InspectorField::FieldType::InputText, &locationTagString});
}

bool PlayerLocationScript::Init()
{
    locationTag = HashString(locationTagString);

    return true;
}

void PlayerLocationScript::OnCollisionEnter(GameObject* otherObject, const float3 collisionNormal, ColliderLayer layer)
{
    AppEngine->GetSceneModule()->GetScene()->SetPlayerPosition(locationTag);

    auto taggedGameObjects = AppEngine->GetSceneModule()->GetScene()->GetTaggedGameObjects(locationTag);
    if (taggedGameObjects)
    {
        for (GameObject* currentGameObject : *taggedGameObjects)
        {
            ScriptComponent* script = currentGameObject->GetComponent<ScriptComponent*>();
            if (script)
            {
                Character* character = script->GetScriptByType<Character>();
                if (character) character->OnPlayerEnterLocation();
            }
        }
    }
}

void PlayerLocationScript::OnCollisionExit(GameObject* otherObject, ColliderLayer layer)
{
    AppEngine->GetSceneModule()->GetScene()->SetPlayerPosition(HashString(""));

    auto taggedGameObjects = AppEngine->GetSceneModule()->GetScene()->GetTaggedGameObjects(locationTag);
    if (taggedGameObjects)
    {
        for (GameObject* currentGameObject : *taggedGameObjects)
        {
            ScriptComponent* script = currentGameObject->GetComponent<ScriptComponent*>();
            if (script)
            {
                Character * character = script->GetScriptByType<Character>();
                if (character) character->OnPlayerExitLocation();
            }
        }
    }
}
