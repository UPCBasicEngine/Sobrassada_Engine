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
    fields.push_back({"Current Sequence", InspectorField::FieldType::Int, &currentSequence, 1, 3});
    fields.push_back(
        {"Trigger Sequence",
         [this](Script* self)
         {
             GLOG("Triggering sequence: %d", currentSequence);
             StartSequence(currentSequence);
         }}
    );
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

             // searches for active objects with a mirage script, adds gameobject references to activate them later
             for (const auto& [uid, gameObject] : gameObjects)
             {
                 if (!gameObject || gameObject->IsEnabled()) continue;

                 ScriptComponent* scriptComp = gameObject->GetComponent<ScriptComponent*>();

                 if (scriptComp && scriptComp->GetScriptByType<Mirage>())
                 {
                     targetSequence->mirageObjects.push_back(gameObject);
                 }
             }
         }}
    );
}
// CAREFUL!!! Searches for gameobjects with a specific name
bool BossMirage::Init()
{

    Scene* scene            = AppEngine->GetSceneModule()->GetScene();

    sequence1.mirageObjects = GetMirageChildren(scene, "Sequence1");
    sequence2.mirageObjects = GetMirageChildren(scene, "Sequence2");
    sequence3.mirageObjects = GetMirageChildren(scene, "Sequence3");

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

std::vector<GameObject*> BossMirage::GetMirageChildren(Scene* scene, const std::string& parentName)
{
    std::vector<GameObject*> result;

    const auto& gameObjects = scene->GetAllGameObjects();

    for (const auto& [uid, go] : gameObjects)
    {
        if (!go || go->GetName() != parentName) continue;

        GLOG("Found parent object: %s", parentName.c_str());

        const auto& childrenUIDs = go->GetChildren();

        for (UID childUID : childrenUIDs)
        {
            auto it = gameObjects.find(childUID);
            if (it == gameObjects.end()) continue;

            GameObject* child = it->second;
            if (!child || child->IsEnabled()) continue;

            ScriptComponent* scriptComp = child->GetComponent<ScriptComponent*>();
            if (scriptComp && scriptComp->GetScriptByType<Mirage>())
            {
                result.push_back(child);
                GLOG("Checking child: %s | Active: %s", child->GetName().c_str(), child->IsEnabled() ? "Yes" : "No");
            }
        }

        break; // found the correct parent object
    }

    return result;
}
