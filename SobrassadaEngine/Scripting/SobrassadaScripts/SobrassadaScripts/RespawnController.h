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
    void Respawn();

    private:
      std::string playerName = "";
      GameObject* player     = nullptr;
      bool isPlayerDead      = false;
      float respawnTimer     = 0.0f;
      float respawnInit      = 1.0f;
      CuChulainn* cuState    = nullptr;

};
