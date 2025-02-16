#include "SceneModule.h"

#include "CameraModule.h"
#include "ComponentUtils.h"
#include "FrustumPlanes.h"
#include "GameObject.h"
#include "Octree.h"

#include "glew.h"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl2.h"

#include "Root/RootComponent.h"
#include "Scene/Components/Standalone/MeshComponent.h"

#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_EXTERNAL_IMAGE
#include "EditorUIModule.h"
#include "LibraryModule.h"

#include <Algorithm/Random/LCG.h>
#include <tiny_gltf.h>

SceneModule::SceneModule()
{
}

SceneModule::~SceneModule()
{
    CloseScene();
}

bool SceneModule::Init()
{
    return true;
}

update_status SceneModule::PreUpdate(float deltaTime)
{
    return UPDATE_CONTINUE;
}

update_status SceneModule::Update(float deltaTime)
{
    return UPDATE_CONTINUE;
}

update_status SceneModule::Render(float deltaTime)
{
    if (loadedScene != nullptr)
    {
        return loadedScene->Render(deltaTime);
    }

    return UPDATE_CONTINUE;
}

update_status SceneModule::RenderEditor(float deltaTime)
{
    if (loadedScene != nullptr)
    {
        return loadedScene->RenderEditor(deltaTime);
    }
    return UPDATE_CONTINUE;
}

update_status SceneModule::PostUpdate(float deltaTime)
{
    return UPDATE_CONTINUE;
}

bool SceneModule::ShutDown()
{
    GLOG("Destroying octree")
    return true;
}

void SceneModule::CreateScene()
{
    CloseScene();

    GameObject* sceneGameObject = new GameObject("SceneModule GameObject");

    loadedScene                 = new Scene(GenerateUID(), "New Scene", sceneGameObject->GetUID());

    std::unordered_map<UID, GameObject*> loadedGameObjects;
    loadedGameObjects.insert({sceneGameObject->GetUID(), sceneGameObject});

    loadedScene->LoadComponents(std::map<UID, Component*>());
    loadedScene->LoadGameObjects(loadedGameObjects);

    sceneGameObject->CreateRootComponent();

    // TODO Filesystem: Save this new created level immediatelly
}

void SceneModule::LoadScene(
    UID sceneUID, const char* sceneName, UID rootGameObject, const std::map<UID, Component*>& loadedGameComponents
)
{
    CloseScene();
    loadedScene = new Scene(sceneUID, sceneName, rootGameObject);
    loadedScene->LoadComponents(loadedGameComponents);
}

void SceneModule::LoadGameObjects(const std::unordered_map<UID, GameObject*>& loadedGameObjects)
{
    loadedScene->LoadGameObjects(loadedGameObjects);
}

void SceneModule::CloseScene()
{
    delete loadedScene;
    loadedScene = nullptr;
}

void SceneModule::SwitchPlayModeStateTo(bool wantedStatePlayMode)
{
    if (wantedStatePlayMode == bInPlayMode) return;

    if (bInPlayMode)
    {
        if (loadedScene != nullptr)
        {
            App->GetLibraryModule()->LoadScene(
                std::string(SCENES_PATH + std::string(loadedScene->GetSceneName()) + SCENE_EXTENSION).c_str(), true
            );
            bInPlayMode = false;
        }
    }
    else
    {
        if (loadedScene != nullptr)
        {
            loadedScene->Save();
            bInPlayMode = true;
        }
    }
}
