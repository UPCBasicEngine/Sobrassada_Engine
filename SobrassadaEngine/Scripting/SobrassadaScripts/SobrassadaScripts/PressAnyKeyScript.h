#pragma once

#include "Script.h"
#include <string>

class PressAnyKeyScript : public Script
{
  public:

    PressAnyKeyScript(GameObject* parent);
    bool Init() override;
    void Update(float deltaTime) override;


  private:
    std::string nextGameObjectName;
};
