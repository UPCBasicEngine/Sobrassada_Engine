#pragma once
#include "Script.h"
#include <string>
#include <vector>

class GameObject;

class GameOverScript : public Script
{
  public:
    explicit GameOverScript(GameObject* parent) : Script(parent) {}
    bool Init() override;
    void Update(float) override;
    void Inspector() override;
    void Save(rapidjson::Value&, rapidjson::Document::AllocatorType&) override;
    void Load(const rapidjson::Value&) override;
    void CloneFields(const std::vector<InspectorField>&) override {}
    void OnCollision(GameObject*, const float3, ColliderLayer) override {}

  private:
    void TriggerGameOver();
    void CachePanel();
    void ShowPanel();
    void PauseGame();

    std::string panelToShowName = "GameOverPanel";
    std::vector<InspectorField> fields {
        {"Panel To Show", InspectorField::FieldType::InputText, &panelToShowName}
    };

    GameObject* cachedTarget = nullptr;
    bool gameOverShown       = false;
};
