#include "pch.h"

#include "Application.h"
#include "EnemySpawnerScript.h"
#include "GameObject.h"
#include "Math/float3.h"
#include "Math/float4x4.h"
#include "PrefabManager.h"
#include "ResourcePrefab.h"
#include "ResourcesModule.h"
#include "Scene.h"
#include "SceneModule.h"
#include "Standalone/AIAgentComponent.h"

EnemySpawnerScript::EnemySpawnerScript(GameObject* parent) : Script(parent)
{
    fields.push_back({"Prefab UID", InspectorField::FieldType::Resource, &prefabUID});
    fields.push_back({"Location Tag", InspectorField::FieldType::InputText, &locationTagString});
    fields.push_back({"Spawn Once", InspectorField::FieldType::Bool, &spawnOnce});
    fields.push_back({"Enemies to Spawn", InspectorField::FieldType::Int, &spawnAmount});
}

EnemySpawnerScript::~EnemySpawnerScript()
{
}

bool EnemySpawnerScript::Init()
{
    prefab      = PrefabManager::LoadPrefab(prefabUID);
    locationTag = HashString(locationTagString);

    if (!prefab) GLOG("[EnemYSpawner - WARNING] No prefab found by uid");
    return true;
}

void EnemySpawnerScript::OnDestroy()
{
    delete prefab;
}

void EnemySpawnerScript::OnCollisionEnter(GameObject* other, const float3 normal, ColliderLayer layer)
{

    if (spawnOnce && spawned) return;
    if (!prefab) return;

    Scene* scene           = AppEngine->GetSceneModule()->GetScene();

    UID parentUID          = parent->GetParent();
    GameObject* spawnRoot  = (parentUID != INVALID_UID) ? scene->GetGameObjectByUID(parentUID) : nullptr;
    float4x4 baseTransform = spawnRoot ? spawnRoot->GetGlobalTransform() : parent->GetGlobalTransform();

    for (int i = 0; i < spawnAmount; ++i)
    {
        float3 offset            = float3(i * 2.0f, 0, 0);
        float4x4 spawnTransform  = baseTransform;

        spawnTransform[0][3]    += offset.x;
        spawnTransform[1][3]    += offset.y;
        spawnTransform[2][3]    += offset.z;

        scene->AddPrefab(prefabUID, spawnTransform, locationTag);
    }

    if (spawnOnce) spawned = true;
}
