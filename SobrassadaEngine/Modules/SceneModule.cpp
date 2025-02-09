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
    delete MOCKUP_loadedModel;
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

    rootComponent = dynamic_cast<RootComponent *>(ComponentUtils::CreateEmptyComponent(COMPONENT_ROOT, LCG().IntFast(), -1, -1, Transform())); // TODO Add the gameObject UUID as parent?
    gameComponents[rootComponent->GetUUID()] = rootComponent;
    
    MOCKUP_loadedModel = new EngineModel();
    MOCKUP_loadedModel->Load("./Test/BakerHouse.gltf");

    const uint32_t bakerHouseID = LCG().IntFast();
    const uint32_t bakerHouseChimneyID = LCG().IntFast();
    
    MOCKUP_loadedMeshes[bakerHouseID] = MOCKUP_loadedModel->GetMesh(1);
    MOCKUP_loadedMeshes[bakerHouseChimneyID] = MOCKUP_loadedModel->GetMesh(0);
    MOCKUP_libraryMeshes["Baker house"] = bakerHouseID;
    MOCKUP_libraryMeshes["Baker house chimney"] = bakerHouseChimneyID;
    
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

    // Drag and Drop functionality
    if (ImGui::BeginDragDropSource())
    {
        ImGui::SetDragDropPayload("DRAG_DROP_GAMEOBJECT", &gameObjectUUID, sizeof(uint32_t));
        ImGui::Text("Dragging %s", GetGameObjectByUUID(gameObjectUUID)->GetName().c_str());
        ImGui::EndDragDropSource();
    }

    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("DRAG_DROP_GAMEOBJECT"))
        {
            uint32_t draggedUUID = *reinterpret_cast<const uint32_t *>(payload->Data);

            if (draggedUUID != gameObjectUUID) UpdateGameObjectHierarchy(draggedUUID, gameObjectUUID);
        }

        ImGui::EndDragDropTarget();
    }
}

void SceneModule::RenderContextMenu(uint32_t gameObjectUUID)
{
    if (ImGui::BeginPopup(("Game Object Context Menu##" + std::to_string(gameObjectUUID)).c_str()))
    {
        if (ImGui::MenuItem("New GameObject"))
        {
            uint32_t newUUID = LCG().IntFast();
            GameObject *newGameObject =
                new GameObject(selectedGameObjectUUID, "new Game Object" + std::to_string(newUUID));

            GetGameObjectByUUID(selectedGameObjectUUID)->AddGameObject(newUUID);

            // TODO: change when filesystem defined
            gameObjectsContainer.insert({newUUID, newGameObject});
        }
        
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

void SceneModule::UpdateGameObjectHierarchy(uint32_t sourceUUID, uint32_t targetUUID)
{
    GameObject *sourceGameObject = GetGameObjectByUUID(sourceUUID);
    GameObject *targetGameObject = GetGameObjectByUUID(targetUUID);

    if (!sourceGameObject || !targetGameObject) return;

    uint32_t oldParentUUID = sourceGameObject->GetParent();
    sourceGameObject->SetParent(targetUUID);

    if (gameObjectsContainer.count(oldParentUUID))
    {
        GameObject *oldParentGameObject = GetGameObjectByUUID(oldParentUUID);
        oldParentGameObject->RemoveGameObject(sourceUUID);
    }

    targetGameObject->AddGameObject(sourceUUID);

}