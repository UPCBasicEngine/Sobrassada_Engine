#include "SceneModule.h"

#include "ComponentUtils.h"
#include "EngineModel.h"
#include "GameObject.h"

#include "glew.h"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl2.h"

#include "Root/RootComponent.h"

#include <tiny_gltf.h>
#include <Algorithm/Random/LCG.h>

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
    
    gameObjectRootUUID = sceneGameObject->GetUID();
    selectedGameObjectUUID      = gameObjectRootUUID;
    sceneGameObject->SetUUID(gameObjectRootUUID);

    //TODO: Change when filesystem defined
    gameObjectsContainer.insert({gameObjectRootUUID, sceneGameObject});

    //DEMO
    GameObject *sceneGameChildObject = new GameObject(gameObjectRootUUID, "SceneModule GameObject child");
    UID gameObjectChildRootUUID      = sceneGameChildObject->GetUID();
    sceneGameChildObject->SetUUID(gameObjectChildRootUUID);
    sceneGameObject->AddGameObject(gameObjectChildRootUUID);

    // TODO: Change when filesystem defined
    gameObjectsContainer.insert({gameObjectChildRootUUID, sceneGameChildObject});
    
    MOCKUP_loadedModel = new EngineModel();
    MOCKUP_loadedModel->Load("./Test/BakerHouse.gltf");

    const UID bakerHouseID = LCG().IntFast();
    const UID bakerHouseChimneyID = LCG().IntFast();

    const UID bakerHouseTextureID = LCG().IntFast();
    
    MOCKUP_loadedMeshes[bakerHouseID] = MOCKUP_loadedModel->GetMesh(1);
    MOCKUP_loadedMeshes[bakerHouseChimneyID] = MOCKUP_loadedModel->GetMesh(0);
    MOCKUP_libraryMeshes["Baker house"] = bakerHouseID;
    MOCKUP_libraryMeshes["Baker house chimney"] = bakerHouseChimneyID;

    // TODO Always have a default grid texture loaded to apply as standard
    MOCKUP_loadedTextures[bakerHouseTextureID] = MOCKUP_loadedModel->GetActiveRenderTexture();
    MOCKUP_libraryTextures["Baker house texture"] = bakerHouseTextureID;
    
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
    for (auto& gameObject: gameObjectsContainer)
    {
        gameObject.second->Render();
    }
    return UPDATE_CONTINUE;
}

update_status SceneModule::RenderEditor(float deltaTime)
{
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
        //UID newUUID          = LCG().IntFast();
        //if (newUUID == INVALID_UUID) newUUID = LCG().IntFast();

        GameObject *newGameObject = new GameObject(selectedGameObjectUUID, "new Game Object");
        UID newUUID               = newGameObject->GetUID();
        //newGameObject->SetUUID(newUUID);
        GetGameObjectByUUID(selectedGameObjectUUID)->AddGameObject(newUUID);

        //TODO: change when filesystem defined
        gameObjectsContainer.insert({newUUID, newGameObject});
    }

    if (selectedGameObjectUUID != gameObjectRootUUID)
    {
        ImGui::SameLine();
        
        if (ImGui::Button("Delete GameObject")) 
            RemoveGameObjectHierarchy(selectedGameObjectUUID);
    }

    RenderGameObjectHierarchy(gameObjectRootUUID);

    ImGui::End();
}

void SceneModule::RenderGameObjectHierarchy(UID gameObjectUUID)
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

    ImGui::PushID(gameObjectUUID);
    bool nodeOpen = ImGui::TreeNodeEx(gameObject->GetName().c_str(), flags);

    HandleNodeClick(gameObjectUUID);
    RenderContextMenu(gameObjectUUID);

    if (nodeOpen && hasChildren)
    {
        for (UID childUUID : gameObject->GetChildren())
        {
            if (childUUID != gameObjectUUID) 
                RenderGameObjectHierarchy(childUUID);
        }

        ImGui::TreePop();
    }

    ImGui::PopID();
}

void SceneModule::HandleNodeClick(UID gameObjectUUID)
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
        ImGui::SetDragDropPayload("DRAG_DROP_GAMEOBJECT", &gameObjectUUID, sizeof(UID));
        ImGui::Text("Dragging %s", GetGameObjectByUUID(gameObjectUUID)->GetName().c_str());
        ImGui::EndDragDropSource();
    }

    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("DRAG_DROP_GAMEOBJECT"))
        {
            UID draggedUUID = *reinterpret_cast<const UID *>(payload->Data);

            if (draggedUUID != gameObjectUUID) UpdateGameObjectHierarchy(draggedUUID, gameObjectUUID);
        }

        ImGui::EndDragDropTarget();
    }
}

void SceneModule::RenderContextMenu(UID gameObjectUUID)
{
    static UID renamingGameObjectUUID = 0;
    static char *newNameBuffer = nullptr;

    if (ImGui::BeginPopup(("Game Object Context Menu##" + std::to_string(gameObjectUUID)).c_str()))
    {
        if (ImGui::MenuItem("New GameObject"))
        {
            UID newUUID = LCG().IntFast();
            GameObject *newGameObject =
                new GameObject(selectedGameObjectUUID, "new Game Object");

            GetGameObjectByUUID(selectedGameObjectUUID)->AddGameObject(newUUID);

            // TODO: change when filesystem defined
            gameObjectsContainer.insert({newUUID, newGameObject});
        }
        
        if (gameObjectUUID != gameObjectRootUUID && ImGui::MenuItem("Delete")) 
            RemoveGameObjectHierarchy(gameObjectUUID);

        
        /*if (ImGui::MenuItem("Rename"))
        {
            renamingGameObjectUUID = gameObjectUUID;
            GameObject *gameObject  = GetGameObjectByUUID(gameObjectUUID);

            if (!newNameBuffer)
            {
                newNameBuffer = new char[128];
                strncpy_s(newNameBuffer, 128, gameObject->GetName().c_str(), _TRUNCATE);
            }
        }*/

        if (gameObjectUUID != gameObjectRootUUID && ImGui::MenuItem("Clear Parent"))
        {
            UpdateGameObjectHierarchy(gameObjectUUID, gameObjectRootUUID);
        }

        ImGui::EndPopup();

        //Renaming Mode
        /*if (renamingGameObjectUUID == gameObjectUUID)
        {
            ImGui::SetNextItemWidth(200.0f);
            if (ImGui::InputText("Rename GameObject", newNameBuffer, 128, ImGuiInputTextFlags_EnterReturnsTrue))
            {
                GameObject *gameObject = GetGameObjectByUUID(gameObjectUUID);
                if (gameObject)
                {
                    gameObject->SetName(newNameBuffer);
                }

                renamingGameObjectUUID = 0;
                delete[] newNameBuffer;
                newNameBuffer = nullptr;
            }

            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsItemHovered())
            {
                renamingGameObjectUUID = 0;
                delete[] newNameBuffer;
                newNameBuffer = nullptr;
            }
        }*/
    }
}

void SceneModule::RemoveGameObjectHierarchy(UID gameObjectUUID)
{
    // TODO: Change when filesystem defined
    if (!gameObjectsContainer.count(gameObjectUUID) || gameObjectUUID == gameObjectRootUUID) return;

    GameObject *gameObject = GetGameObjectByUUID(gameObjectUUID);

    for (UID childUUID : gameObject->GetChildren())
    {
        RemoveGameObjectHierarchy(childUUID);
    }

    UID parentUUID = gameObject->GetParent();

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

void SceneModule::UpdateGameObjectHierarchy(UID sourceUUID, UID targetUUID)
{
    GameObject *sourceGameObject = GetGameObjectByUUID(sourceUUID);
    GameObject *targetGameObject = GetGameObjectByUUID(targetUUID);

    if (!sourceGameObject || !targetGameObject) return;

    UID oldParentUUID = sourceGameObject->GetParent();
    sourceGameObject->SetParent(targetUUID);

    if (gameObjectsContainer.count(oldParentUUID))
    {
        GameObject *oldParentGameObject = GetGameObjectByUUID(oldParentUUID);
        oldParentGameObject->RemoveGameObject(sourceUUID);
    }

    targetGameObject->AddGameObject(sourceUUID);

}

AABBUpdatable * SceneModule::GetTargetForAABBUpdate(UID uuid)
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