#pragma once
#include "GameTimer.h"
#include "Script.h"

class GameOverScript final : public Script
{
  public:
    GameOverScript(GameObject* owner) : Script(owner) {}

    bool Init() override;
    void Update(float dt) override;
    void OnPlayerDeath();     
    void Inspector() override; 
    void Save(rapidjson::Value&, rapidjson::Document::AllocatorType&) override;
    void Load(const rapidjson::Value&) override;

  private:
   
    float showDelay      = 1.0f;   
    GameObject* canvasGO = nullptr;

    bool pending         = false;
    float timer          = 0.0f;
};
