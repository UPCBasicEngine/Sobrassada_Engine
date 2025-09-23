#pragma once
#include "Script.h"
#include <cstdint>
#include <string>
#include <vector>

class GameObject;

class GameOverScript : public Script
{
  public:
    GameOverScript(GameObject* parent);

    bool Init() override;
    void Update(float dt) override;
    void Inspector() override;

    // Open / close
    void TriggerGameOver();
    void Close();

    // Accessors (for external navigation)
    GameObject* GetPanelRoot() const { return panelRoot; }
    int GetSelectedIndex() const { return selectedIndex; }
    void SetSelectedIndex(int idx)
    {
        selectedIndex = idx;
        UpdateSelection();
    }

  private:
    void CachePanel();
    void BuildFromPanel();
    void DisableAllArrows();
    void UpdateSelection();

    // Inspector-exposed fields
    std::string panelToShowName = "GameOverPanel";
    std::vector<InspectorField> fields {
        {"Panel To Show", InspectorField::FieldType::InputText, &panelToShowName}
    };

    // UI state
    GameObject* panelRoot = nullptr;
    std::vector<GameObject*> menuItems;
    std::vector<GameObject*> arrowImages;

    int selectedIndex  = 0;
    bool builtOnce     = false;
    bool gameOverShown = false;
};

// Global flag
extern bool gGameOverActive;

// C bridge
extern "C" void GO_RequestGameOver();
