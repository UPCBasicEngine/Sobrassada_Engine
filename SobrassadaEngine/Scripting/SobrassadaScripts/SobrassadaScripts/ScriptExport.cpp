#include "pch.h"

#include "ButtonScript.h"
#include "CameraMovement.h"
#include "CuChulainn.h"
#include "ExitGameScript.h"
#include "FireballTrap.h"
#include "FreeCamera.h"
#include "FullscreenToggleScript.h"
#include "Globals.h"
#include "GodMode.h"
#include "MainMenuSelectorScript.h"
#include "OptionsMenuSwitcherScript.h"
#include "PauseMenuScript.h"
#include "PressAnyKeyScript.h"
#include "Projectile.h"
#include "RotateGameObject.h"
#include "Soldier.h"
#include "SpawnPoint.h"
#include "VSyncToggleScript.h"

#include <string>

#ifndef SOBRASSADASCRIPTS_EXPORTS
#define SOBRASSADA_API __declspec(dllexport)
#else
#define SOBRASSADA_API __declspec(dllimport)
#endif

constexpr const char* scripts[] = {
    "RotateGameObjectScript",
    "ButtonScript",
    "GodModeScript",
    "CuChulainnScript",
    "SoldierScript",
    "ExitGameScript",
    "FullscreenToggleScript",
    "VSyncToggleScript",
    "PauseMenuScript",
    "OptionsMenuSwitcherScript",
    "MainMenuSelectorScript",
    "PressAnyKeyScript",
    "CameraMovement",
    "Projectile",
    "FreeCamera",
    "SpawnPoint",
    "FireballTrap"
};

Application* AppEngine = nullptr;
extern "C" SOBRASSADA_API void InitSobrassadaScripts(Application* App)
{
    // GLOG("Sobrassada Scripts Initialized");
    AppEngine = App;
}

extern "C" SOBRASSADA_API Script* CreateScript(const std::string& scriptType, GameObject* parent)
{
    /* UI */
    if (scriptType == "ButtonScript") return new ButtonScript(parent);
    if (scriptType == "ExitGameScript") return new ExitGameScript(parent);
    if (scriptType == "FullscreenToggleScript") return new FullscreenToggleScript(parent);
    if (scriptType == "VSyncToggleScript") return new VSyncToggleScript(parent);
    if (scriptType == "PauseMenuScript") return new PauseMenuScript(parent);
    if (scriptType == "OptionsMenuSwitcherScript") return new OptionsMenuSwitcherScript(parent);
    if (scriptType == "MainMenuSelectorScript") return new MainMenuSelectorScript(parent);
    if (scriptType == "PressAnyKeyScript") return new PressAnyKeyScript(parent);

    /* Characters */
    if (scriptType == "CuChulainnScript") return new CuChulainn(parent);
    if (scriptType == "SoldierScript") return new Soldier(parent);
    if (scriptType == "CameraMovement") return new CameraMovement(parent);
    if (scriptType == "Projectile") return new Projectile(parent);
    if (scriptType == "SpawnPoint") return new SpawnPoint(parent);
    if (scriptType == "FireballTrap") return new FireballTrap(parent);

    /* Utils */
    if (scriptType == "RotateGameObjectScript") return new RotateGameObject(parent);
    if (scriptType == "GodModeScript") return new GodMode(parent);
    if (scriptType == "FreeCamera") return new FreeCamera(parent);

    return nullptr;
}

extern "C" SOBRASSADA_API void DestroyScript(Script* script)
{
    delete script;
}

extern "C" SOBRASSADA_API void FreeSobrassadaScripts()
{
    // GLOG("Sobrassada Scripts deleted");
    AppEngine = nullptr;
}

extern "C" SOBRASSADA_API const int GetScriptCount()
{
    return sizeof(scripts) / sizeof(scripts[0]);
}

extern "C" SOBRASSADA_API const char* GetScriptName(const int index)
{
    if (index < 0 || index >= GetScriptCount()) return nullptr;
    return scripts[index];
}

extern "C" SOBRASSADA_API const int GetScriptIndexByName(const std::string& scriptString)
{
    for (int i = 0; i < GetScriptCount(); ++i)
    {
        if (scriptString == scripts[i])
        {
            return i;
        }
    }
    return 0;
}