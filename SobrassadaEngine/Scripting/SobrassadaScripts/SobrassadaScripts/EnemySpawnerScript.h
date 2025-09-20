#pragma once

#include "HashString.h"
#include "Script.h"

#include <string>

class GameObject;

class EnemySpawnerScript : public Script
{
  public:
    explicit EnemySpawnerScript(GameObject* parent);
    ~EnemySpawnerScript() override;

    bool Init() override;
    void OnDestroy() override;
    void Update(float deltaTime) override {};
    void OnCollisionEnter(GameObject* otherObject, const float3 collisionNormal, ColliderLayer layer) override;

  private:
    UID prefabUID                 = INVALID_UID;

    std::string locationTagString = "";
    HashString locationTag;

    bool spawnOnce         = false;
    bool spawned           = false;
    int spawnAmount        = 1;
};
