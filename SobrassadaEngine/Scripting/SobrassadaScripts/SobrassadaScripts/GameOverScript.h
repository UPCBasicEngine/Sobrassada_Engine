#pragma once
#include "Script.h"
#include <string>
#include <vector>

class GameObject;

class GameOverScript : public Script
{
  public:
    GameOverScript(GameObject* parent);

    bool Init() override;
    void Update(float) override;
    void Close();

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
