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
#include "EditorViewport.h"

#include <Algorithm/Random/LCG.h>
#include <tiny_gltf.h>

SceneModule::SceneModule()
{
}

SceneModule::~SceneModule()
{
}

bool SceneModule::Init()
{
    GLOG("Creating SceneModule renderer context");

    sceneUID                    = 0;
    sceneName                   = "Default Scene";

    GameObject* sceneGameObject = new GameObject("SceneModule GameObject");

    gameObjectRootUUID          = sceneGameObject->GetUID();
    selectedGameObjectUUID      = gameObjectRootUUID;
    sceneGameObject->SetUUID(gameObjectRootUUID);

    // TODO: Change when filesystem defined
    gameObjectsContainer.insert({gameObjectRootUUID, sceneGameObject});

    // DEMO
    GameObject* sceneGameChildObject = new GameObject(gameObjectRootUUID, "SceneModule GameObject child");
    UID gameObjectChildRootUUID      = sceneGameChildObject->GetUID();
    sceneGameChildObject->SetUUID(gameObjectChildRootUUID);
    sceneGameObject->AddGameObject(gameObjectChildRootUUID);

    // TODO: Change when filesystem defined
    gameObjectsContainer.insert({gameObjectChildRootUUID, sceneGameChildObject});

    lightsConfig = new LightsConfig();
    lightsConfig->InitSkybox();
    lightsConfig->InitLightBuffers();

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
    // Render skybox and lights
    lightsConfig->RenderSkybox();
    lightsConfig->SetLightsShaderData();

    //TODO: Integrate with editor UI
    lightsConfig->EditorParams();
    
    for (auto it = gameObjectsContainer.begin(); it != gameObjectsContainer.end(); it++)
    {
        if (it->second != nullptr)
        {
            it->second->Render();
        }
        else
        {
            GLOG("Empty gameObject in scene detected")
        }
    }

    // Probably should go somewhere else, but must go after skybox and meshes
    App->GetDebugDrawModule()->Draw();

    return UPDATE_CONTINUE;
}

update_status SceneModule::RenderEditor(float deltaTime)
{
    App->GetEditorUIModule()->editorViewport->Render();
    GameObject* selectedGameObject = gameObjectsContainer[selectedGameObjectUUID];
    if (selectedGameObject != nullptr)
    {
        selectedGameObject->RenderEditor();
    }
    return UPDATE_CONTINUE;
}

update_status SceneModule::PostUpdate(float deltaTime)
{
    return UPDATE_CONTINUE;
}

bool SceneModule::ShutDown()
{
    delete sceneOctree;

    GLOG("Destroying octree");
    return true;
}

void SceneModule::LoadScene(UID sceneUID, const char* sceneName, UID rootGameObject)
{
    this->sceneUID         = sceneUID;
    gameObjectRootUUID     = rootGameObject;
    selectedGameObjectUUID = gameObjectRootUUID;
    this->sceneName        = sceneName;

    lightsConfig           = new LightsConfig();
    lightsConfig->InitSkybox();
    lightsConfig->InitLightBuffers();
}

void SceneModule::CloseScene()
{
    for (const auto& [uid, gameObject] : gameObjectsContainer)
    {
        delete gameObject;
    }
    gameObjectsContainer.clear();

    for (const auto& [uid, component] : gameComponents)
    {
        delete component;
    }
    gameComponents.clear();

    gameObjectRootUUID     = 0;
    selectedGameObjectUUID = 0;

    delete lightsConfig;
    lightsConfig = nullptr;

    GLOG("%s scene closed", sceneName.c_str());
}

void SceneModule::CreateSpatialDataStruct()
{
    // float3 octreeCenter = float3::zero;
    // float octreeLength  = 100;
    // int nodeCapacity    = 5;
    // sceneOctree         = new Octree(octreeCenter, octreeLength, nodeCapacity);

    // for (auto& objectIterator : gameObjectsContainer)
    //{
    //     AABB objectBB = objectIterator.second->GetGlobalBoundingBox();

    //    if (objectBB.Size().x == 0 && objectBB.Size().y == 0 && objectBB.Size().z == 0) continue;

    //    sceneOctree->InsertElement(objectIterator.second);
    //}
}

void SceneModule::UpdateSpatialDataStruct()
{
    delete sceneOctree;

    CreateSpatialDataStruct();
}

void SceneModule::CheckObjectsToRender(std::vector<GameObject*>& outRenderGameObjects) const
{
    // std::vector<GameObject*> queriedObjects;
    // const FrustumPlanes& frustumPlanes = App->GetCameraModule()->GetFrustrumPlanes();

    // sceneOctree->QueryElements(frustumPlanes, queriedObjects);

    // for (auto gameObject : queriedObjects)
    //{
    //     OBB objectOBB = gameObject->GetGlobalOrientedBoundingBox();

    //    if (frustumPlanes.Intersects(objectOBB)) outRenderGameObjects.push_back(gameObject);
    //}
}

void SceneModule::RenderHierarchyUI(bool& hierarchyMenu)
{
    ImGui::Begin("Hierarchy", &hierarchyMenu);

    if (ImGui::Button("Add GameObject"))
    {
        GameObject* newGameObject = new GameObject(selectedGameObjectUUID, "new Game Object");
        UID newUUID               = newGameObject->GetUID();

        GetGameObjectByUUID(selectedGameObjectUUID)->AddGameObject(newUUID);

        // TODO: change when filesystem defined
        gameObjectsContainer.insert({newUUID, newGameObject});

        GetGameObjectByUUID(newGameObject->GetParent())->ComponentGlobalTransformUpdated();
    }

    if (selectedGameObjectUUID != gameObjectRootUUID)
    {
        ImGui::SameLine();

        if (ImGui::Button("Delete GameObject"))
        {
            UID parentUID                = GetGameObjectByUUID(selectedGameObjectUUID)->GetParent();
            GameObject* parentGameObject = GetGameObjectByUUID(parentUID);
            RemoveGameObjectHierarchy(selectedGameObjectUUID);
            // parentGameObject->PassAABBUpdateToParent(); // TODO: check if it works
        }
    }

    GameObject* rootGameObject = GetGameObjectByUUID(gameObjectRootUUID);
    if (rootGameObject)
    {
        rootGameObject->RenderHierarchyNode(selectedGameObjectUUID);
    }

    ImGui::End();
}

void SceneModule::RemoveGameObjectHierarchy(UID gameObjectUUID)
{
    // TODO: Change when filesystem defined
    if (!gameObjectsContainer.count(gameObjectUUID) || gameObjectUUID == gameObjectRootUUID) return;

    GameObject* gameObject = GetGameObjectByUUID(gameObjectUUID);

    for (UID childUUID : gameObject->GetChildren())
    {
        RemoveGameObjectHierarchy(childUUID);
    }

    UID parentUUID = gameObject->GetParent();

    // TODO: change when filesystem defined
    if (gameObjectsContainer.count(parentUUID))
    {
        GameObject* parentGameObject = GetGameObjectByUUID(parentUUID);
        parentGameObject->RemoveGameObject(gameObjectUUID);
        selectedGameObjectUUID = parentUUID;
    }

    // TODO: change when filesystem defined
    gameObjectsContainer.erase(gameObjectUUID);

    delete gameObject;
}

AABBUpdatable* SceneModule::GetTargetForAABBUpdate(UID uuid)
{
    if (gameObjectsContainer.count(uuid))
    {
        return gameObjectsContainer[uuid];
    }

    if (gameComponents.count(uuid))
    {
        return gameComponents[uuid];
    }

    return nullptr;
}

GameObject* SceneModule::GetGameObjectByUUID(UID gameObjectUUID)
{
    if (gameObjectsContainer.count(gameObjectUUID))
    {
        return gameObjectsContainer[gameObjectUUID];
    }
    return nullptr;
}