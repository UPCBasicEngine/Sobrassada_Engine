#include "SceneModule.h"
#include "GameObject.h"

#include "glew.h"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl2.h"

#include "Algorithm/Random/LCG.h"

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
        GameObject *newGameObject = new GameObject(selectedGameObjectUUID, "new Game Object##" + std::to_string(newUUID));
        
        gameObjectsContainer[selectedGameObjectUUID]->AddGameObject(newUUID);

        gameObjectsContainer.insert({newUUID, newGameObject});
    }
    
    ImGui::SameLine();

    if (ImGui::Button("Delete GameObject") && selectedGameObjectUUID != gameObjectRootUUID)
    {
        RemoveGameObjectHierarchy(selectedGameObjectUUID);
        selectedGameObjectUUID = gameObjectRootUUID;
    }

    RenderGameObjectHierarchy(gameObjectRootUUID, selectedGameObjectUUID);

    ImGui::End();
}

void SceneModule::RenderGameObjectHierarchy(uint32_t gameObjectUUID, uint32_t& selectedGameObjectUUID)
{
    if (!gameObjectsContainer.count(gameObjectUUID)) return;

    GameObject *gameObject = gameObjectsContainer[gameObjectUUID];
    
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;

    if (gameObject->GetChildren().size() == 0)
    {
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }

    if (selectedGameObjectUUID == gameObjectUUID) flags |= ImGuiTreeNodeFlags_Selected;

    if (ImGui::TreeNodeEx(gameObject->GetName().c_str(), flags))
    {
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
        {
            selectedGameObjectUUID = gameObjectUUID;
        }

        for (uint32_t childUUID : gameObject->GetChildren())
        {
            if (childUUID != gameObjectUUID) RenderGameObjectHierarchy(childUUID, selectedGameObjectUUID);
        }

        if (gameObject->GetChildren().size() > 0)
        {
            ImGui::TreePop();
        }

    }
    else
    {
        if (ImGui::IsItemClicked()) selectedGameObjectUUID = gameObjectUUID;
    }

}

void SceneModule::RemoveGameObjectHierarchy(uint32_t gameObjectUUID)
{ 
    if (!gameObjectsContainer.count(gameObjectUUID)) return;

    GameObject *gameObject = gameObjectsContainer[gameObjectUUID];

    for (uint32_t childUUID : gameObject->GetChildren())
    {
        RemoveGameObjectHierarchy(childUUID);
    }

    uint32_t parentUUID = gameObject->GetParent();

    if (gameObjectsContainer.count(parentUUID))
    {
        GameObject *parentGameObject = gameObjectsContainer[parentUUID];
        parentGameObject->RemoveGameObject(gameObjectUUID);
    }

    gameObjectsContainer.erase(gameObjectUUID);

    delete gameObject;

}