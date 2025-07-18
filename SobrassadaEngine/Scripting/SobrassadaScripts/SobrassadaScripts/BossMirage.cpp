#include "pch.h"
#include "BossMirage.h"
#include "Application.h"
#include "GameObject.h"
#include "Mirage.h"
#include "RotateGameObject.h"
#include "SceneModule.h"
#include "ScriptComponent.h"


BossMirage::BossMirage(GameObject* parent) : Script(parent)
{
    std::vector<Mirage*> foundMirages;
    fields.push_back({"Current Sequence", InspectorField::FieldType::Int, &currentSequence, 1, 4});
    fields.push_back(
        {"Gather Sequence",
         [this](Script* self)
         {
             AttackSequence newSequence;

             const auto& gameObjects = App->GetSceneModule()->GetScene()->GetAllGameObjects();

             for (const auto& [uid, gameObject] : gameObjects)
             {
                 if (!gameObject || !gameObject->IsEnabled()) continue;

                 ScriptComponent* scriptComp = gameObject->GetComponent<ScriptComponent*>();
                 if (scriptComp && scriptComp->GetScriptByType<Mirage>())
                 {
                     newSequence.mirageObjects.push_back(gameObject);
                 }
             }

             sequences.push_back(std::move(newSequence));
         }}
    );
}

bool BossMirage::Init()
{
    return false;
}

void BossMirage::Update(float deltaTime)
{
}
