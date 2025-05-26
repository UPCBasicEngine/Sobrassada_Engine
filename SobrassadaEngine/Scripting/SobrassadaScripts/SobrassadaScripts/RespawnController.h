#pragma once
#include "Script.h"

class GameObject;
class CuChulainn;
class RespawnController : public Script
{
  public:
    RespawnController(GameObject* parent);
    virtual ~RespawnController() noexcept override { parent = nullptr; }

    bool Init() override;
    void Update(float deltaTime) override;
   

  private:
    std::string playerName = "";
    GameObject* player     = nullptr;
    bool isPlayerDead      = false;
    float respawnTimer     = 0.0f;
    float deathTimer       = 0.5f;
    float respawnInit      = 5.0f;
   
};