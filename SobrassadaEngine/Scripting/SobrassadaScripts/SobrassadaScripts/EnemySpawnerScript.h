#pragma once

#include "Script.h"
#include <string>

class GameObject;

class EnemySpawnerScript : public Script
{
  public:
    explicit EnemySpawnerScript(GameObject* parent);
    ~EnemySpawnerScript() noexcept override { parent = nullptr; }

    bool Init() override;
    void Update(float /*deltaTime*/) override {} // no per-frame logic
    void OnCollision(GameObject* otherObject, const float3 collisionNormal, ColliderLayer layer) override;

    void Save(rapidjson::Value& tgt, rapidjson::Document::AllocatorType& al);

    void Load(const rapidjson::Value& src);

  private:
    std::string prefabUIDStr = ""; // text que pots editar a l’Inspector
    UID prefabUID            = INVALID_UID;

    bool spawnOnce           = false;
    bool spawned             = false;
};
