#include "pch.h"

#include "Archer.h"
#include "ArcherProjectile.h"
#include "Banshee.h"
#include "Banshee_v2.h"
#include "Boss.h"
#include "ButtonScript.h"
#include "CameraMovement.h"
#include "ChangeSceneScript.h"
#include "Changeling.h"
#include "CuChulainn.h"
#include "Destructible.h"
#include "EnemySpawnerScript.h"
#include "ExitGameScript.h"
#include "FireballTrap.h"
#include "FreeCamera.h"
#include "FullscreenToggleScript.h"
#include "GameOverScript.h"
#include "Globals.h"
#include "GodMode.h"
#include "HealVFXGround.h"
#include "MagicBarrier.h"
#include "MainMenuSelectorScript.h"
#include "MenuChangeSceneScript.h"
#include "MiniFireball.h"
#include "MirageVFX.h"
#include "MoveGOInSpline.h"
#include "Mushroom.h"
#include "OptionsMenuSwitcherScript.h"
#include "PauseMenuScript.h"
#include "PlayerLocationScript.h"
#include "PressAnyKeyScript.h"
#include "Projectile.h"
#include "RotateGameObject.h"
#include "Soldier.h"
#include "SpawnPoint.h"
#include "SpawnUI.h"
#include "Spouts.h"
#include "SwitchScriptTest.h"
#include "TileFloatScript.h"
#include "VSyncToggleScript.h"
#include "WallCollision.h"

#include "AbilityIconFill.h"
#include "AttackVfxSpritesheet.h"
#include "BarFill.h"
#include "DamageMask.h"
#include "MovingUVClipErode.h"
#include "MovingUVLight.h"
#include "MovingUVPostScript.h"
#include "MovingUVTransparent.h"
#include "VSyncToggleScript.h"

#include "BossMirage.h"
#include "Mirage.h"
#include "MirageBossDash.h"

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
    "MiniFireball",
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
    "Spouts",
    "SwitchScriptTest",
    "Destructible",
    "MagicBarrier",
    "Banshee_v2",
    "WallCollision",
    "Mirage",
    "BossMirage",
    "Boss",
    "MirageBossDash",
    "ArcherProjectile"
};

constexpr const char* shaderScripts[] = {
    "MovingUVPostScript", "MovingUVLight",   "MovingUVTransparent",   "HealGroundHalo",
    "HealVerticalPlanes", "HealSpikesBurst", "HealGroundSpikesLight", "HealGroundSpikesDark",
    "HealLightBurst",     "HealSpikesUp",    "RiastradBarFill",       "HealthBarFill",
    "AbilityIconFill",    "DamageMask",      "AttackVfxSpritesheet",  "MovingUVClipErode",
    "MirageVFX"
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
    if (scriptType == "ArcherProjectile") return new ArcherProjectile(parent);
    if (scriptType == "Banshee") return new Banshee(parent);
    if (scriptType == "Archer") return new Archer(parent);
    if (scriptType == "Changeling") return new Changeling(parent);
    if (scriptType == "Banshee_v2") return new Banshee_v2(parent);
    if (scriptType == "Boss") return new Boss(parent);

    /* Environment */
    if (scriptType == "TileFloatScript") return new TileFloatScript(parent);
    if (scriptType == "FireballTrap") return new FireballTrap(parent);
    if (scriptType == "MiniFireball") return new MiniFireball(parent);
    if (scriptType == "Mushroom") return new Mushroom(parent);
    if (scriptType == "SpawnPoint") return new SpawnPoint(parent);
    if (scriptType == "EnemySpawnerScript") return new EnemySpawnerScript(parent);
    if (scriptType == "PlayerLocationScript") return new PlayerLocationScript(parent);
    if (scriptType == "Spouts") return new Spouts(parent);
    if (scriptType == "Destructible") return new Destructible(parent);
    if (scriptType == "MagicBarrier") return new MagicBarrier(parent);
    if (scriptType == "WallCollision") return new WallCollision(parent);

    /* Utils */
    if (scriptType == "RotateGameObjectScript") return new RotateGameObject(parent);
    if (scriptType == "GodModeScript") return new GodMode(parent);
    if (scriptType == "ChangeSceneScript") return new ChangeSceneScript(parent);
    if (scriptType == "FreeCamera") return new FreeCamera(parent);
    if (scriptType == "MoveGOInSpline") return new MoveGOInSpline(parent);
    if (scriptType == "SwitchScriptTest") return new SwitchScriptTest(parent);

    /* Render Scripts */
    if (scriptType == "MovingUVPostScript") return new MovingUVPostScript(parent);
    if (scriptType == "MovingUVLight") return new MovingUVLight(parent);
    if (scriptType == "MovingUVTransparent") return new MovingUVTransparent(parent);
    if (scriptType == "AttackVfxSpritesheet") return new AttackVfxSpritesheet(parent);
    if (scriptType == "DamageMask") return new DamageMask(parent);
    if (scriptType == "RiastradBarFill")
        return new BarFill(parent, "./EngineDefaults/Shader/Custom/Fragment/UI_RiastradBarFill.glsl");
    if (scriptType == "HealthBarFill")
        return new BarFill(parent, "./EngineDefaults/Shader/Custom/Fragment/UI_HealthBarFill.glsl");
    if (scriptType == "AbilityIconFill") return new AbilityIconFill(parent);
    if (scriptType == "MovingUVClipErode") return new MovingUVClipErode(parent);
    if (scriptType == "HealGroundHalo")
        return new HealVFXGround(
            parent, "./EngineDefaults/Shader/Custom/Vertex/HealVFX_Vertex.glsl",
            "./EngineDefaults/Shader/Custom/Fragment/HealVFX/Heal_GroundHalo.glsl"
        );

    if (scriptType == "HealVerticalPlanes")
        return new HealVFXGround(
            parent, "./EngineDefaults/Shader/Custom/Vertex/HealVFX_Vertex.glsl",
            "./EngineDefaults/Shader/Custom/Fragment/HealVFX/Heal_VerticalPlanes.glsl"
        );

    if (scriptType == "HealSpikesBurst")
        return new HealVFXGround(
            parent, "./EngineDefaults/Shader/Custom/Vertex/HealVFX_Vertex.glsl",
            "./EngineDefaults/Shader/Custom/Fragment/HealVFX/Heal_SpikesBurst.glsl"
        );

    if (scriptType == "HealGroundSpikesLight")
        return new HealVFXGround(
            parent, "./EngineDefaults/Shader/Custom/Vertex/HealVFX_Vertex.glsl",
            "./EngineDefaults/Shader/Custom/Fragment/HealVFX/Heal_GroundSpikesLight.glsl"
        );

    if (scriptType == "HealGroundSpikesDark")
        return new HealVFXGround(
            parent, "./EngineDefaults/Shader/Custom/Vertex/HealVFX_Vertex.glsl",
            "./EngineDefaults/Shader/Custom/Fragment/HealVFX/Heal_GroundSpikesDark.glsl"
        );

    if (scriptType == "HealLightBurst")
        return new HealVFXGround(
            parent, "./EngineDefaults/Shader/Custom/Vertex/HealVFX_Vertex.glsl",
            "./EngineDefaults/Shader/Custom/Fragment/HealVFX/Heal_LightBurst.glsl"
        );

    if (scriptType == "HealSpikesUp")
        return new HealVFXGround(
            parent, "./EngineDefaults/Shader/Custom/Vertex/HealVFX_Vertex.glsl",
            "./EngineDefaults/Shader/Custom/Fragment/HealVFX/Heal_SpikesUp.glsl"
        );

    /*Boss*/
    if (scriptType == "Mirage") return new Mirage(parent);
    if (scriptType == "BossMirage") return new BossMirage(parent);
    if (scriptType == "MirageBossDash") return new MirageBossDash(parent);

    if (scriptType == "MirageVFX")
        return new HealVFXGround(
            parent, "./EngineDefaults/Shader/Custom/Vertex/MirageVFX_Vertex.glsl",
            "./EngineDefaults/Shader/Custom/Fragment/MirageVFX_Fragment.glsl"
        );

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

extern "C" SOBRASSADA_API const int GetShaderScriptCount()
{
    return sizeof(shaderScripts) / sizeof(shaderScripts[0]);
}

extern "C" SOBRASSADA_API const char* GetShaderScriptName(const int index)
{
    if (index < 0 || index >= GetShaderScriptCount()) return nullptr;
    return shaderScripts[index];
}

extern "C" SOBRASSADA_API const int GetShaderScriptIndexByName(const std::string& scriptString)
{
    for (int i = 0; i < GetScriptCount(); ++i)
    {
        if (scriptString == shaderScripts[i])
        {
            return i;
        }
    }
    return 0;
}