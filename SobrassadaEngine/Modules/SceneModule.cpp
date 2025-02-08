#include "SceneModule.h"

#include "ComponentUtils.h"
#include "EngineModel.h"
#include "GameObject.h"

#include "glew.h"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl2.h"

#include "Algorithm/Random/LCG.h"
#include "Root/RootComponent.h"

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

    GameObject* sceneGameObject = new GameObject("SceneModule GameObject");
    gameObjectRootUUID = LCG().IntFast();

    gameObjectsContainer.insert({gameObjectRootUUID, sceneGameObject});

    //DEMO
    GameObject *sceneGameChildObject = new GameObject(gameObjectRootUUID, "SceneModule GameObject child");
    uint32_t gameObjectChildRootUUID = LCG().IntFast();

    sceneGameObject->AddGameObject(gameObjectChildRootUUID);

    gameObjectsContainer.insert({gameObjectChildRootUUID, sceneGameChildObject});

    rootComponent = dynamic_cast<RootComponent *>(ComponentUtils::CreateEmptyComponent(COMPONENT_ROOT, LCG().IntFast(), -1, -1, Transform())); // TODO Add the gameObject UUID as parent?
    gameComponents[rootComponent->GetUUID()] = rootComponent;

    const uint32_t newModelID = LCG().IntFast();
    EngineModel* model = new EngineModel();
    model->Load("./Test/BakerHouse.gltf");
    loadedModels[newModelID] = model;
    MOCKUP_libraryModels["Baker house"] = newModelID;
    
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
    if (rootComponent != nullptr)
    {
        rootComponent->Render();
    }
    return UPDATE_CONTINUE;
}

update_status SceneModule::RenderEditor(float deltaTime)
{
    if (rootComponent)  rootComponent->RenderComponentEditor();
    return UPDATE_CONTINUE;
}

update_status SceneModule::PostUpdate(float deltaTime)
{
    return UPDATE_CONTINUE;
}

bool SceneModule::ShutDown()
{
    GLOG("Destroying renderer");
    return true;
}

void SceneModule::LoadScene()
{
}

void SceneModule::CloseScene()
{
}

void SceneModule::CreateSpatialDataStruct()
{
}

void SceneModule::UpdateSpatialDataStruct()
{
}

void SceneModule::CheckObjectsToRender()
{
}

void SceneModule::RenderHierarchyUI(bool &hierarchyMenu) 
{
    static uint32_t selectedGameObjectUUID = gameObjectRootUUID;

    ImGui::Begin("Hierarchy", &hierarchyMenu);

    if (ImGui::Button("Add GameObject"))
    {
        uint32_t newUUID          = LCG().IntFast();
        GameObject *newGameObject = new GameObject(selectedGameObjectUUID, "new Game Object" + std::to_string(newUUID));
        
        gameObjectsContainer[selectedGameObjectUUID]->AddGameObject(newUUID);

        gameObjectsContainer.insert({newUUID, newGameObject});
    }
    
    ImGui::SameLine();

    if(ImGui::Button("Delete GameObject"));
    //Make basic implementation and remember to call the function
    //instance of moduleScene is required

    RenderGameObjectHierarchy(gameObjectRootUUID, selectedGameObjectUUID);

    ImGui::End();
}

void SceneModule::RenderGameObjectHierarchy(uint32_t gameObjectUUID, uint32_t& selectedGameObjectUUID)
{
    if (!gameObjectsContainer.count(gameObjectUUID)) return;

    GameObject *gameObject = gameObjectsContainer[gameObjectUUID];

    if (ImGui::TreeNode(gameObject->GetName().c_str()))
    {
        if (ImGui::IsItemClicked()) selectedGameObjectUUID = gameObjectUUID;

        for (uint32_t childUUID : gameObject->GetChildren())
        {
            if (childUUID != gameObjectUUID) RenderGameObjectHierarchy(childUUID, selectedGameObjectUUID);
        }

        ImGui::TreePop();
    }
    else
    {
        if (ImGui::IsItemClicked()) selectedGameObjectUUID = gameObjectUUID;
    }

}
