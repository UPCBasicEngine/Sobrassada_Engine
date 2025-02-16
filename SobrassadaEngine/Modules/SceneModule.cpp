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
    delete sceneOctree;

    GLOG("Destroying octree")
    return true;
}

void SceneModule::LoadScene(UID sceneUID, const char* sceneName, UID rootGameObject,
    const std::map<UID, Component*> &loadedGameComponents,
    const std::unordered_map<UID, GameObject*>& loadedGameObjects)
{
    delete loadedScene;
    loadedScene = new Scene(sceneUID, sceneName, rootGameObject);
    loadedScene->Load(loadedGameComponents,loadedGameObjects);
}

void SceneModule::CloseScene()
{
    delete loadedScene;
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
