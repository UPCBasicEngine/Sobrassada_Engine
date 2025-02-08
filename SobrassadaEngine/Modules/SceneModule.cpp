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
    selectedGameObjectUUID      = gameObjectRootUUID;

    //TODO: Change when filesystem defined
    gameObjectsContainer.insert({gameObjectRootUUID, sceneGameObject});

    //DEMO
    GameObject *sceneGameChildObject = new GameObject(gameObjectRootUUID, "SceneModule GameObject child");
    uint32_t gameObjectChildRootUUID = LCG().IntFast();

    sceneGameObject->AddGameObject(gameObjectChildRootUUID);

    // TODO: Change when filesystem defined
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
    ImGui::Begin("Hierarchy", &hierarchyMenu);

    if (ImGui::Button("Add GameObject"))
    {
        uint32_t newUUID          = LCG().IntFast();
        GameObject *newGameObject = new GameObject(selectedGameObjectUUID, "new Game Object" + std::to_string(newUUID));
        
        GetGameObjectByUUID(selectedGameObjectUUID)->AddGameObject(newUUID);

        //TODO: change when filesystem defined
        gameObjectsContainer.insert({newUUID, newGameObject});
    }

    ImGui::SameLine();

    if (ImGui::Button("Delete GameObject") && selectedGameObjectUUID != gameObjectRootUUID)
    {
        RemoveGameObjectHierarchy(selectedGameObjectUUID);
        //selectedGameObjectUUID = gameObjectRootUUID;
    }

    RenderGameObjectHierarchy(gameObjectRootUUID);

    ImGui::End();
}

void SceneModule::RenderGameObjectHierarchy(uint32_t gameObjectUUID)
{
    // TODO: Change when filesystem defined
    if (!gameObjectsContainer.count(gameObjectUUID)) return;

    GameObject *gameObject = GetGameObjectByUUID(gameObjectUUID);
    
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;

    bool hasChildren = !gameObject->GetChildren().empty();

    if (!hasChildren)
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

    if (selectedGameObjectUUID == gameObjectUUID) 
        flags |= ImGuiTreeNodeFlags_Selected;

    bool nodeOpen = ImGui::TreeNodeEx(gameObject->GetName().c_str(), flags);

    HandleNodeClick(gameObjectUUID);
    RenderContextMenu(gameObjectUUID);

    if (nodeOpen && hasChildren)
    {
        for (uint32_t childUUID : gameObject->GetChildren())
        {
            if (childUUID != gameObjectUUID) 
                RenderGameObjectHierarchy(childUUID);
        }

        ImGui::TreePop();
    }


}

void SceneModule::HandleNodeClick(uint32_t gameObjectUUID)
{
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
    {
        selectedGameObjectUUID = gameObjectUUID;
    }

    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
    {
        selectedGameObjectUUID = gameObjectUUID;
        ImGui::OpenPopup(("Game Object Context Menu##" + std::to_string(gameObjectUUID)).c_str());
    }
}

void SceneModule::RenderContextMenu(uint32_t gameObjectUUID)
{
    if (ImGui::BeginPopup(("Game Object Context Menu##" + std::to_string(gameObjectUUID)).c_str()))
    {
        if (gameObjectUUID != gameObjectRootUUID && ImGui::MenuItem("Delete")) 
            RemoveGameObjectHierarchy(gameObjectUUID);

        ImGui::EndPopup();
    }
}

void SceneModule::RemoveGameObjectHierarchy(uint32_t gameObjectUUID)
{
    // TODO: Change when filesystem defined
    if (!gameObjectsContainer.count(gameObjectUUID) || gameObjectUUID == gameObjectRootUUID) return;

    GameObject *gameObject = GetGameObjectByUUID(gameObjectUUID);

    for (uint32_t childUUID : gameObject->GetChildren())
    {
        RemoveGameObjectHierarchy(childUUID);
    }

    uint32_t parentUUID = gameObject->GetParent();

    //TODO: change when filesystem defined
    if (gameObjectsContainer.count(parentUUID))
    {
        GameObject *parentGameObject = GetGameObjectByUUID(parentUUID);
        parentGameObject->RemoveGameObject(gameObjectUUID);
        selectedGameObjectUUID = parentUUID;
    }

    // TODO: change when filesystem defined
    gameObjectsContainer.erase(gameObjectUUID);

    delete gameObject;

}