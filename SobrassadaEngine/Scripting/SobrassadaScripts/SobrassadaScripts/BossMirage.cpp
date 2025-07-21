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

             AttackSequence* targetSequence = nullptr;

             switch (currentSequence)
             {
             case 1:
                 targetSequence = &sequence1;
                 break;
             case 2:
                 targetSequence = &sequence2;
                 break;
             case 3:
                 targetSequence = &sequence3;
                 break;
             default:
                 return;
             }

             targetSequence->mirageObjects.clear();

             const auto& gameObjects = AppEngine->GetSceneModule()->GetScene()->GetAllGameObjects();

             //searches for active objects with a mirage script, adds gameobject references to activate them later
             for (const auto& [uid, gameObject] : gameObjects)
             {
                 if (!gameObject || !gameObject->IsEnabled()) continue;

                 ScriptComponent* scriptComp = gameObject->GetComponent<ScriptComponent*>();

                 if (scriptComp && scriptComp->GetScriptByType<Mirage>())
                 {
                     targetSequence->mirageObjects.push_back(gameObject);
                 }
             }

         }}
    );
}

bool BossMirage::Init()
{
    return true;
}

void BossMirage::Update(float deltaTime)
{
    {
        if (state == SequenceState::PlayingSequence && sequence)
        {
            timeSinceLastActivation += deltaTime;

            if (currentMirageIndex < sequence->mirageObjects.size())
            {
                if (timeSinceLastActivation >= sequence->delayBetweenZones)
                {
                    GameObject* mirage = sequence->mirageObjects[currentMirageIndex];
                    if (mirage) mirage->SetEnabled(true); // triggers the Mirage logic
                    currentMirageIndex++;
                    timeSinceLastActivation = 0.f;
                }
            }
            else
            {
                // Sequence complete
                state = SequenceState::Idle;
            }
        }
    }
}

void BossMirage::StartSequence(int sequenceNum)
{
    switch (sequenceNum)
    {
    case 1:
        sequence = &sequence1;
        break;
    case 2:
        sequence = &sequence2;
        break;
    case 3:
        sequence = &sequence3;
        break;
    default:
        return;
    }

    currentMirageIndex      = 0;
    timeSinceLastActivation = 0.0f;
    state                   = SequenceState::PlayingSequence;
}
