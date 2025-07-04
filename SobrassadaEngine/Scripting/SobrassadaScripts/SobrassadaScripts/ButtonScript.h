#pragma once

#include "Script.h"

#include <rapidjson/document.h>
#include <string>
#include <list>
#include "Delegate.h"


class ButtonScript : public Script
{
  public:
    ButtonScript(GameObject* parent) : Script(parent) {}
    ~ButtonScript() override;

    bool Init() override;
    void Update(float deltaTime) override;

    void OnClick();
    std::string GetCurrentPanelName() const;
    void Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator);
    void Load(const rapidjson::Value& initialState) override;
    void OnDestroy() override;
    void Inspector() override;


  private:
    std::string panelToHideName = "MainMenuPanel";
    std::string panelToShowName = "OptionsMenuManager";

    std::list<Delegate<void>>::iterator delegateID;
    bool hasRegisteredCallback = false;
};
