#include "pch.h"

#include "Application.h"
#include "EnemySpawnerScript.h"
#include "GameObject.h"
#include "Math/float3.h"
#include "Math/float4x4.h"
#include "PrefabManager.h"
#include "ResourcePrefab.h"
#include "Scene.h"
#include "SceneModule.h"
#include "Standalone/AIAgentComponent.h"

EnemySpawnerScript::EnemySpawnerScript(GameObject* parent) : Script(parent)
{
    fields.push_back({"Prefab UID", InspectorField::FieldType::InputText, &prefabUIDStr});
    fields.push_back({"Spawn Once", InspectorField::FieldType::Bool, &spawnOnce});
    fields.push_back({"Enemies to Spawn", InspectorField::FieldType::Int, &spawnAmount});
}

bool EnemySpawnerScript::Init()
{
    if (!prefabUIDStr.empty()) prefabUID = std::stoull(prefabUIDStr);

    return true;
}

void EnemySpawnerScript::Update(float deltatime)
{
    wasOverlapping   = isOverlappingNow; // remember state of current frame
    isOverlappingNow = false;            // will be set again by OnCollision()
}

void EnemySpawnerScript::OnCollision(GameObject* other, const float3 normal, ColliderLayer layer)
{
    isOverlappingNow = true;

    if (spawnOnce && spawned) return;
    if (wasOverlapping) return;

    if (prefabUID == INVALID_UID && !prefabUIDStr.empty()) prefabUID = std::stoull(prefabUIDStr);
    if (prefabUID == INVALID_UID)
    {
        GLOG("EnemySpawner: Prefab UID not set");
        return;
    }

    ResourcePrefab* prefab = PrefabManager::LoadPrefab(prefabUID);
    if (!prefab)
    {
        GLOG("EnemySpawner: Could not find prefab %llu", prefabUID);
        return;
    }

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

        scene->LoadPrefab(prefabUID, prefab, spawnTransform, true);
    }

    if (spawnOnce) spawned = true;
}




void EnemySpawnerScript::Save(rapidjson::Value& tgt, rapidjson::Document::AllocatorType& al)
{
    if (!prefabUIDStr.empty()) prefabUID = std::stoull(prefabUIDStr);
    tgt.AddMember("PrefabUID", static_cast<uint64_t>(prefabUID), al);
    tgt.AddMember("SpawnOnce", spawnOnce, al);
    tgt.AddMember("SpawnAmount", spawnAmount, al);
}

void EnemySpawnerScript::Load(const rapidjson::Value& src)
{
    if (src.HasMember("PrefabUID"))
    {
        prefabUID    = src["PrefabUID"].GetUint64();
        prefabUIDStr = std::to_string(prefabUID);
    }
    if (src.HasMember("SpawnOnce")) spawnOnce = src["SpawnOnce"].GetBool();
    if (src.HasMember("SpawnAmount")) spawnAmount = src["SpawnAmount"].GetInt();
}
