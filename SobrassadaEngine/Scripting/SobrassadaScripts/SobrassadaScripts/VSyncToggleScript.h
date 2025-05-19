#pragma once

#include "Script.h"
#include "Utils/Delegate.h"
#include <list>

class VSyncToggleScript : public Script
{
  public:
    VSyncToggleScript(GameObject* parent) : Script(parent) {}
    ~VSyncToggleScript() override;


    bool Init() override;
    void Update(float deltaTime) override;
    void OnDestroy() override;


    void OnClick();

  private:
    std::list<Delegate<void>>::iterator delegateID;
    bool hasRegisteredCallback = false;
};
