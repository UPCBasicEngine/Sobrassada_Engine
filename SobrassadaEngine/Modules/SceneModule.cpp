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
#include <tiny_gltf.h>
#include <Algorithm/Random/LCG.h>

SceneModule::SceneModule() {}

SceneModule::~SceneModule() {}

bool SceneModule::Init()
{
    GLOG("Creating SceneModule renderer context");

    sceneUID                    = 0;
    sceneName                   = "Default Scene";
    
    GameObject* sceneGameObject = new GameObject("SceneModule GameObject");
    
    gameObjectRootUUID = sceneGameObject->GetUID();
    selectedGameObjectUUID      = gameObjectRootUUID;
    sceneGameObject->SetUUID(gameObjectRootUUID);

    // TODO: Change when filesystem defined
    gameObjectsContainer.insert({gameObjectRootUUID, sceneGameObject});

    // DEMO
    GameObject *sceneGameChildObject = new GameObject(gameObjectRootUUID, "SceneModule GameObject child");
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

update_status SceneModule::PreUpdate(float deltaTime) { return UPDATE_CONTINUE; }

update_status SceneModule::Update(float deltaTime) { return UPDATE_CONTINUE; }

update_status SceneModule::Render(float deltaTime)
{
    // Render skybox and lights
    lightsConfig->RenderSkybox();
    lightsConfig->SetLightsShaderData();

    for (auto &gameObject : gameObjectsContainer)
    {
        gameObject.second->Render();
    }

    //Probably should go somewhere else, but must go after skybox and meshes
    App->GetDebugDreawModule()->Draw();

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

update_status SceneModule::PostUpdate(float deltaTime) { return UPDATE_CONTINUE; }

bool SceneModule::ShutDown()
{
    delete sceneOctree;

    GLOG("Destroying octree");
    return true;
}

void SceneModule::LoadScene(UID sceneUID, const char *sceneName, UID rootGameObject)
{
    this->sceneUID         = sceneUID;
    gameObjectRootUUID     = rootGameObject;
    selectedGameObjectUUID = gameObjectRootUUID;
    this->sceneName        = sceneName;
}

void SceneModule::CloseScene()
{
    for (const auto &[uid, gameObject] : gameObjectsContainer)
    {
        delete gameObject;
    }
    gameObjectsContainer.clear();

    for (const auto &[uid, component] : gameComponents)
    {
        delete component;
    }
    gameComponents.clear();

    gameObjectRootUUID     = 0;
    selectedGameObjectUUID = 0;

    lightsConfig           = nullptr;

    GLOG("%s scene closed", sceneName.c_str());
}

void SceneModule::CreateSpatialDataStruct()
{
    //float3 octreeCenter = float3::zero;
    //float octreeLength  = 100;
    //int nodeCapacity    = 5;
    //sceneOctree         = new Octree(octreeCenter, octreeLength, nodeCapacity);

    //for (auto& objectIterator : gameObjectsContainer)
    //{
    //    AABB objectBB = objectIterator.second->GetGlobalBoundingBox();

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
    //std::vector<GameObject*> queriedObjects;
    //const FrustumPlanes& frustumPlanes = App->GetCameraModule()->GetFrustrumPlanes();

    //sceneOctree->QueryElements(frustumPlanes, queriedObjects);

    //for (auto gameObject : queriedObjects)
    //{
    //    OBB objectOBB = gameObject->GetGlobalOrientedBoundingBox();

    //    if (frustumPlanes.Intersects(objectOBB)) outRenderGameObjects.push_back(gameObject);
    //}
}



void SceneModule::RenderHierarchyUI(bool& hierarchyMenu)
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

        // TODO: change when filesystem defined
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

    GameObject* gameObject   = GetGameObjectByUUID(gameObjectUUID);

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;

    bool hasChildren         = !gameObject->GetChildren().empty();

    if (!hasChildren) flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

    if (selectedGameObjectUUID == gameObjectUUID) flags |= ImGuiTreeNodeFlags_Selected;

    ImGui::PushID(gameObjectUUID);
    bool nodeOpen = ImGui::TreeNodeEx(gameObject->GetName().c_str(), flags);

    HandleNodeClick(gameObjectUUID);
    RenderContextMenu(gameObjectUUID);

    if (nodeOpen && hasChildren)
    {
        for (UID childUUID : gameObject->GetChildren())
        {
            if (childUUID != gameObjectUUID) RenderGameObjectHierarchy(childUUID);
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
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DRAG_DROP_GAMEOBJECT"))
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

        // Renaming Mode
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

void SceneModule::UpdateGameObjectHierarchy(UID sourceUUID, UID targetUUID)
{
    GameObject* sourceGameObject = GetGameObjectByUUID(sourceUUID);
    GameObject* targetGameObject = GetGameObjectByUUID(targetUUID);

    if (!sourceGameObject || !targetGameObject) return;

    UID oldParentUUID = sourceGameObject->GetParent();
    sourceGameObject->SetParent(targetUUID);

    if (gameObjectsContainer.count(oldParentUUID))
    {
        GameObject* oldParentGameObject = GetGameObjectByUUID(oldParentUUID);
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