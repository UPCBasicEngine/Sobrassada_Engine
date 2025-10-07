#pragma once
#include "Script.h"

class MusicManager : public Script
{
  public:
    MusicManager(GameObject* parent);

    bool Init() override { return true; }
    void Update(float deltaTime) override {}

    void OnPlayerRespawn() const;

};