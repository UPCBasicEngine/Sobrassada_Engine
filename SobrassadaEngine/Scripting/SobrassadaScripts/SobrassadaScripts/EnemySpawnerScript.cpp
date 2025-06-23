#include "pch.h"

#include "Application.h"
#include "EnemySpawnerScript.h"
#include "GameObject.h"
#include "PrefabManager.h"
#include "ResourcePrefab.h"
#include "Scene.h"
#include "SceneModule.h"

EnemySpawnerScript::EnemySpawnerScript(GameObject* parent) : Script(parent)
{
    fields.push_back({"Prefab UID", InspectorField::FieldType::InputText, &prefabUIDStr});
    fields.push_back({"Spawn Once", InspectorField::FieldType::Bool, &spawnOnce});
}

bool EnemySpawnerScript::Init()
{
    if (!prefabUIDStr.empty()) prefabUID = std::stoull(prefabUIDStr);

    return true;
}

void EnemySpawnerScript::OnCollision(GameObject* otherObject, const float3 collisionNormal, ColliderLayer layer)
{
    if (spawnOnce && spawned) 
        return;

    if (prefabUID == INVALID_UID && !prefabUIDStr.empty()) prefabUID = std::stoull(prefabUIDStr);

    if (prefabUID == INVALID_UID)
    {
        GLOG("EnemySpawner: Prefab UID no definit");
        return;
    }

    ResourcePrefab* prefab = PrefabManager::LoadPrefab(prefabUID);
    if (!prefab)
    {
        GLOG("EnemySpawner: no trobo el prefab %llu", prefabUID);
        return;
    }

    Scene* scene     = AppEngine->GetSceneModule()->GetScene();
    float4x4 spawnTf = parent->GetGlobalTransform();     
    scene->LoadPrefab(prefabUID, prefab, spawnTf, true);
   
    GLOG("EnemySpawner: instanciat prefab %llu", prefabUID);
    spawned = true;
}

void EnemySpawnerScript::Save(rapidjson::Value& tgt, rapidjson::Document::AllocatorType& al)
{
    if (!prefabUIDStr.empty()) prefabUID = std::stoull(prefabUIDStr);

    tgt.AddMember("PrefabUID", static_cast<uint64_t>(prefabUID), al);
    tgt.AddMember("SpawnOnce", spawnOnce, al);
}

void EnemySpawnerScript::Load(const rapidjson::Value& src)
{
    if (src.HasMember("PrefabUID"))
    {
        prefabUID    = src["PrefabUID"].GetUint64();
        prefabUIDStr = std::to_string(prefabUID);
    }
    if (src.HasMember("SpawnOnce")) spawnOnce = src["SpawnOnce"].GetBool();
}
