#pragma once

#include "Script.h"

class GameObject;

class AttackVfx : public Script
{
  public:
    AttackVfx(GameObject* parent);
    virtual ~AttackVfx() noexcept override { parent = nullptr; }

    bool Init() override;
    void Update(float deltaTime) override;

  private:
    std::string cameraName = "";
    GameObject* camera     = nullptr;

    bool fullBillboard     = false;
};