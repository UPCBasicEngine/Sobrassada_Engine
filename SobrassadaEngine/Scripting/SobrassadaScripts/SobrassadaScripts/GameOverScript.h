#pragma once
#include "Script.h"
#include <string>
#include <vector>

class GameObject;

class GameOverScript final : public Script
{
  public:
    explicit GameOverScript(GameObject* parent) : Script(parent) {}
    ~GameOverScript() override = default;

    bool Init() override;
    void Update(float deltaTime) override;
    void Inspector() override;
    void Save(rapidjson::Value& state, rapidjson::Document::AllocatorType& alloc) override;
    void Load(const rapidjson::Value& initialState) override;

    void OnCollision(GameObject*, const float3, ColliderLayer) override {}

    void TriggerGameOver();

  private:
    void CachePanel();
    void ShowPanel();
    void PauseGame();

    std::string panelToShowName        = "GameOverPanel";
    std::vector<InspectorField> fields = {
        {"Panel To Show", InspectorField::FieldType::InputText, &panelToShowName}
    };

    GameObject* cachedTarget = nullptr;
    bool gameOverShown       = false;
};
