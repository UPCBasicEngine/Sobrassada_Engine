#include "pch.h"

#include "Banshee.h"
#include "ButtonScript.h"
#include "CameraMovement.h"
#include "CuChulainn.h"
#include "ExitGameScript.h"
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
    if (scriptType == "FreeCamera") return new FreeCamera(parent);

    /* Characters */
    if (scriptType == "CuChulainnScript") return new CuChulainn(parent);
    if (scriptType == "SoldierScript") return new Soldier(parent);
    if (scriptType == "CameraMovement") return new CameraMovement(parent);
    if (scriptType == "Projectile") return new Projectile(parent);
    if (scriptType == "SpawnPoint") return new SpawnPoint(parent);
    if (scriptType == "Banshee") return new Banshee(parent);

    /* Utils */
    if (scriptType == "RotateGameObject") return new RotateGameObject(parent);
    if (scriptType == "GodMode") return new GodMode(parent);
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