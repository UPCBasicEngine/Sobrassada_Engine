#pragma once

#include "Module.h"
#include "GameObject.h"
#include "Globals.h"

#include <ft2build.h>
#include <vector>
#include FT_FREETYPE_H


namespace TextManager
{
    struct FontData;
}

class CanvasComponent;

class GameUIModule : public Module
{
  public:
    GameUIModule();
    ~GameUIModule() override;

    bool Init() override;
    update_status Update(float deltaTime) override;
    update_status Render(float deltaTime) override;
    bool ShutDown() override;

    void OnWindowResize(const unsigned int width, const unsigned int height);
    void AddCanvas(CanvasComponent* newCanvas) { canvases.push_back(newCanvas); }
    void RemoveCanvas(CanvasComponent* canvasToRemove);

    void RegisterScreen(const std::string& screenName, const std::vector<UID>& gameObjects);
    void RegisterScreenFromChildren(const std::string& screenName, const std::string& parentGOName);
    void SwitchToScreen(const std::string& screenName);

    bool SOBRASADA_API_ENGINE HasShownIntroScreen() const { return hasShownIntroScreen; }
    void SOBRASADA_API_ENGINE SetIntroScreenShown(bool shown) { hasShownIntroScreen = shown; }

  private:
    GameObject* FindGameObjectByName(const std::string& name);

    std::vector<CanvasComponent*> canvases;

    std::unordered_map<std::string, std::vector<UID>> screens;
    std::string currentScreen;
    bool hasShownIntroScreen = false;
};