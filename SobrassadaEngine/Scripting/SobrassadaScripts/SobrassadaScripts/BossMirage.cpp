#include "pch.h"

#include "Application.h"
#include "BossMirage.h"
#include "GameObject.h"
#include "Mirage.h"
#include "SceneModule.h"
#include "ScriptComponent.h"

BossMirage::BossMirage(GameObject* parent) : Script(parent)
{
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
             targetSequence->waves.clear();

             const auto& gameObjects = AppEngine->GetSceneModule()->GetScene()->GetAllGameObjects();

             for (const auto& [uid, gameObject] : gameObjects)
             {
                 if (!gameObject) continue;

                 ScriptComponent* scriptComp = gameObject->GetComponent<ScriptComponent*>();
                 Mirage* mirage              = scriptComp ? scriptComp->GetScriptByType<Mirage>() : nullptr;

                 if (mirage)
                 {
                     targetSequence->mirageObjects.push_back(gameObject);
                     targetSequence->waves[mirage->getOrder()].push_back(gameObject);
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

    // Build wave maps
    auto buildWaves         = [](AttackSequence& seq)
    {
        seq.waves.clear();
        for (GameObject* go : seq.mirageObjects)
        {
            if (!go) continue;
            ScriptComponent* sc = go->GetComponent<ScriptComponent*>();
            Mirage* mirage      = sc ? sc->GetScriptByType<Mirage>() : nullptr;
            if (mirage) seq.waves[mirage->getOrder()].push_back(go);
        }
    };

    buildWaves(sequence1);
    buildWaves(sequence2);
    buildWaves(sequence3);

    sequence3.delayBetweenZones = 1.0f;

    return true;
}

void BossMirage::Update(float deltaTime)
{
    if (state == SequenceState::PlayingSequence && sequence)
    {
        timeSinceLastActivation += deltaTime;

        if (timeSinceLastActivation >= sequence->delayBetweenZones)
        {
            auto it = sequence->waves.find(currentWeightOrder);
            if (it != sequence->waves.end())
            {
                // Activate all mirages with this weightOrder at once
                for (GameObject* mirage : it->second)
                {
                    GLOG("Now playing mirage (weight %d): %s", currentWeightOrder, mirage->GetName().c_str());
                    if (mirage) mirage->SetEnabled(true);
                }

                // Move to next weight order
                ++currentWeightOrder;
                timeSinceLastActivation = 0.f;
            }
            else
            {
                // No more waves
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

    currentWeightOrder      = 1; // start at lowest order
    timeSinceLastActivation = 0.0f;
    state                   = SequenceState::PlayingSequence;
}

// SEQUENCE PARENT NEEDS TO BE ENABLED
std::vector<GameObject*> BossMirage::GetMirageChildren(Scene* scene, const std::string& parentName)
{
    std::vector<GameObject*> result;
    const auto& gameObjects = scene->GetAllGameObjects();

    for (const auto& [uid, go] : gameObjects)
    {
        if (!go || go->GetName() != parentName) continue;

        const auto& childrenUIDs = go->GetChildren();
        for (UID childUID : childrenUIDs)
        {
            auto it = gameObjects.find(childUID);
            if (it == gameObjects.end()) continue;

            GameObject* child           = it->second;
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