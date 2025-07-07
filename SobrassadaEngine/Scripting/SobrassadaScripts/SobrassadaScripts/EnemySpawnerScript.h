#pragma once

#include "Script.h"
#include "HashString.h"

#include <string>

class GameObject;
class ResourcePrefab;

class EnemySpawnerScript : public Script
{
  public:
    explicit EnemySpawnerScript(GameObject* parent);
    ~EnemySpawnerScript() noexcept override { parent = nullptr; }

    bool Init() override;
    void Update(float deltaTime) override;
    void OnCollision(GameObject* otherObject, const float3 collisionNormal, ColliderLayer layer) override;

    void Save(rapidjson::Value& tgt, rapidjson::Document::AllocatorType& al);

    void Load(const rapidjson::Value& src);

  private:
    std::string prefabUIDStr      = ""; // Enemy UID
    UID prefabUID                 = INVALID_UID;

    std::string locationTagString = "";
    HashString locationTag;

    ResourcePrefab* prefab = nullptr;

    bool spawnOnce         = false;
    bool spawned           = false;
    int spawnAmount        = 1;

    bool wasOverlapping    = false; // inside during previous frame
    bool isOverlappingNow  = false; // inside at least once this frame
};
