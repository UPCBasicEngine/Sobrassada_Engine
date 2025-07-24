#include "pch.h"

#include "Archer.h"
#include "Changeling.h"
#include "Banshee.h"
#include "ButtonScript.h"
#include "CameraMovement.h"
#include "ChangeSceneScript.h"
#include "CuChulainn.h"
#include "ExitGameScript.h"
#include "FireballTrap.h"
#include "FreeCamera.h"
#include "FullscreenToggleScript.h"
#include "Globals.h"
#include "GodMode.h"
#include "MainMenuSelectorScript.h"
#include "MenuChangeSceneScript.h"
#include "MoveGOInSpline.h"
#include "Mushroom.h"
#include "OptionsMenuSwitcherScript.h"
#include "PauseMenuScript.h"
#include "PressAnyKeyScript.h"
#include "Projectile.h"
#include "RotateGameObject.h"
#include "Soldier.h"
#include "SpawnPoint.h"
#include "SpawnUI.h"
#include "TileFloatScript.h"
#include "VSyncToggleScript.h"
#include "EnemySpawnerScript.h"
#include "GameOverScript.h"
#include "PlayerLocationScript.h"
#include "Destructible.h"

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
    "Banshee",
    "TileFloatScript",
    "FireballTrap",
    "Archer",
    "Changeling",
    "ChangeSceneScript",
    "SpawnUI",
    "MenuChangeSceneScript",
    "MoveGOInSpline",
    "Mushroom",
    "EnemySpawnerScript",
    "GameOverScript",
    "PlayerLocationScript",
    "Destructible"
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
    if (scriptType == "SpawnUI") return new SpawnUI(parent);
    if (scriptType == "MenuChangeSceneScript") return new MenuChangeSceneScript(parent);
    if (scriptType == "GameOverScript") return new GameOverScript(parent);

    /* Characters */
    if (scriptType == "CuChulainnScript") return new CuChulainn(parent);
    if (scriptType == "SoldierScript") return new Soldier(parent);
    if (scriptType == "CameraMovement") return new CameraMovement(parent);
    if (scriptType == "Projectile") return new Projectile(parent);
    if (scriptType == "Banshee") return new Banshee(parent);
    if (scriptType == "Archer") return new Archer(parent);
    if (scriptType == "Changeling") return new Changeling(parent);

    /* Environment */
    if (scriptType == "TileFloatScript") return new TileFloatScript(parent);
    if (scriptType == "FireballTrap") return new FireballTrap(parent);
    if (scriptType == "Mushroom") return new Mushroom(parent);
    if (scriptType == "SpawnPoint") return new SpawnPoint(parent);
    if (scriptType == "EnemySpawnerScript") return new EnemySpawnerScript(parent);
    if (scriptType == "PlayerLocationScript") return new PlayerLocationScript(parent);
    if (scriptType == "Destructible") return new Destructible(parent);

    /* Utils */
    if (scriptType == "RotateGameObjectScript") return new RotateGameObject(parent);
    if (scriptType == "GodModeScript") return new GodMode(parent);
    if (scriptType == "ChangeSceneScript") return new ChangeSceneScript(parent);
    if (scriptType == "FreeCamera") return new FreeCamera(parent);
    if (scriptType == "MoveGOInSpline") return new MoveGOInSpline(parent);

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