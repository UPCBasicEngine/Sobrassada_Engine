#pragma once
#include "Script.h"
#include "Delegate.h"
#include <list>

class FullscreenToggleScript : public Script
{
  public:
    FullscreenToggleScript(GameObject* parent) : Script(parent) {}
    ~FullscreenToggleScript() override;
    void OnDestroy() override;
    bool Init() override;
    void Update(float deltaTime) override;

    void OnClick();

  private:
    std::list<Delegate<void>>::iterator delegateID;
    bool hasRegisteredCallback = false;
};
