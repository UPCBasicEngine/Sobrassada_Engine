#include "SceneModule.h"

#include "CameraModule.h"
#include "ComponentUtils.h"
#include "EngineModel.h"
#include "FrustumPlanes.h"
#include "GameObject.h"
#include "Octree.h"

#include "glew.h"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl2.h"

#include "Root/RootComponent.h"
#include "Scene/Components/Standalone/MeshComponent.h"

#include <Algorithm/Random/LCG.h>
#include <tiny_gltf.h>

// SPATIAL_PARTITIONING TESTING
#include "Application.h"
#include "CameraModule.h"
#include "DebugDrawModule.h"
#include "Framebuffer.h"
#include "Geometry/LineSegment.h"
#include "Geometry/OBB.h"
#include "Math/Quat.h"
#include "OpenGLModule.h"

SceneModule::SceneModule()
{
}

SceneModule::~SceneModule()
{
    delete MOCKUP_loadedModel;

    for (auto gameObject : gameObjectsContainer)
    {
        delete gameObject.second;
    }
}

bool SceneModule::Init()
{
    GLOG("Creating SceneModule renderer context");

    GameObject* sceneGameObject = new GameObject("SceneModule GameObject");

    gameObjectRootUUID          = LCG().IntFast();
    selectedGameObjectUUID      = gameObjectRootUUID;
    sceneGameObject->SetUUID(gameObjectRootUUID);

    // TODO: Change when filesystem defined
    gameObjectsContainer.insert({gameObjectRootUUID, sceneGameObject});

    // DEMO
    GameObject* sceneGameChildObject = new GameObject(gameObjectRootUUID, "SceneModule GameObject child");
    uint32_t gameObjectChildRootUUID = LCG().IntFast();
    sceneGameChildObject->SetUUID(gameObjectChildRootUUID);
    sceneGameObject->AddGameObject(gameObjectChildRootUUID);

    // TODO: Change when filesystem defined
    gameObjectsContainer.insert({gameObjectChildRootUUID, sceneGameChildObject});

    MOCKUP_loadedModel = new EngineModel();
    MOCKUP_loadedModel->Load("./Test/BakerHouse.gltf");

    const uint32_t bakerHouseID                   = LCG().IntFast();
    const uint32_t bakerHouseChimneyID            = LCG().IntFast();

    const uint32_t bakerHouseTextureID            = LCG().IntFast();

    MOCKUP_loadedMeshes[bakerHouseID]             = MOCKUP_loadedModel->GetMesh(1);
    MOCKUP_loadedMeshes[bakerHouseChimneyID]      = MOCKUP_loadedModel->GetMesh(0);
    MOCKUP_libraryMeshes["Baker house"]           = bakerHouseID;
    MOCKUP_libraryMeshes["Baker house chimney"]   = bakerHouseChimneyID;

    // TODO Always have a default grid texture loaded to apply as standard
    MOCKUP_loadedTextures[bakerHouseTextureID]    = MOCKUP_loadedModel->GetActiveRenderTexture();
    MOCKUP_libraryTextures["Baker house texture"] = bakerHouseTextureID;

    // SPATIAL_PARTITIONING TESTING
    CreateHouseGameObject(2000);
    CreateSpatialDataStruct();

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
    std::vector<GameObject*> objectsToRender;

    CheckObjectsToRender(objectsToRender);

    for (const auto gameObject : objectsToRender)
    {
        gameObject->Render();
    }

    //RenderBoundingBoxes();
    //RenderOctree();

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
    delete sceneOctree;

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
    float3 octreeCenter = float3::zero;
    float octreeLength  = 100;
    int nodeCapacity    = 5;
    sceneOctree         = new Octree(octreeCenter, octreeLength, nodeCapacity);

    for (auto& objectIterator : gameObjectsContainer)
    {
        AABB objectBB = objectIterator.second->GetGlobalBoundingBox();

        if (objectBB.Size().x == 0 && objectBB.Size().y == 0 && objectBB.Size().z == 0) continue;

        sceneOctree->InsertElement(objectIterator.second);
    }
}

void SceneModule::UpdateSpatialDataStruct()
{
    delete sceneOctree;

    CreateSpatialDataStruct();
}

void SceneModule::CheckObjectsToRender(std::vector<GameObject*>& outRenderGameObjects) const
{
    std::vector<GameObject*> queriedObjects;
    const FrustumPlanes& frustumPlanes = App->GetCameraModule()->GetFrustrumPlanes();

    sceneOctree->QueryElements(frustumPlanes, queriedObjects);

    for (auto gameObject : queriedObjects)
    {
        OBB objectOBB = gameObject->GetGlobalOrientedBoundingBox();

        if (frustumPlanes.Intersects(objectOBB)) outRenderGameObjects.push_back(gameObject);
    }
}

void SceneModule::RenderHierarchyUI(bool& hierarchyMenu)
{
    ImGui::Begin("Hierarchy", &hierarchyMenu);

    if (ImGui::Button("Add GameObject"))
    {
        uint32_t newUUID          = LCG().IntFast();
        GameObject* newGameObject = new GameObject(selectedGameObjectUUID, "new Game Object");
        newGameObject->SetUUID(newUUID);
        GetGameObjectByUUID(selectedGameObjectUUID)->AddGameObject(newUUID);

        // TODO: change when filesystem defined
        gameObjectsContainer.insert({newUUID, newGameObject});
    }

    ImGui::SameLine();

    if (ImGui::Button("Delete GameObject") && selectedGameObjectUUID != gameObjectRootUUID)
    {
        RemoveGameObjectHierarchy(selectedGameObjectUUID);
        // selectedGameObjectUUID = gameObjectRootUUID;
    }

    RenderGameObjectHierarchy(gameObjectRootUUID);

    ImGui::End();
}

void SceneModule::RenderGameObjectHierarchy(uint32_t gameObjectUUID)
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
        for (uint32_t childUUID : gameObject->GetChildren())
        {
            if (childUUID != gameObjectUUID) RenderGameObjectHierarchy(childUUID);
        }

        ImGui::TreePop();
    }

    ImGui::PopID();
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
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DRAG_DROP_GAMEOBJECT"))
        {
            uint32_t draggedUUID = *reinterpret_cast<const uint32_t*>(payload->Data);

            if (draggedUUID != gameObjectUUID) UpdateGameObjectHierarchy(draggedUUID, gameObjectUUID);
        }

        ImGui::EndDragDropTarget();
    }
}

void SceneModule::RenderContextMenu(uint32_t gameObjectUUID)
{
    static uint32_t renamingGameObjectUUID = 0;
    static char* newNameBuffer             = nullptr;

    if (ImGui::BeginPopup(("Game Object Context Menu##" + std::to_string(gameObjectUUID)).c_str()))
    {
        if (ImGui::MenuItem("New GameObject"))
        {
            uint32_t newUUID          = LCG().IntFast();
            GameObject* newGameObject = new GameObject(selectedGameObjectUUID, "new Game Object");

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

void SceneModule::RemoveGameObjectHierarchy(uint32_t gameObjectUUID)
{
    // TODO: Change when filesystem defined
    if (!gameObjectsContainer.count(gameObjectUUID) || gameObjectUUID == gameObjectRootUUID) return;

    GameObject* gameObject = GetGameObjectByUUID(gameObjectUUID);

    for (uint32_t childUUID : gameObject->GetChildren())
    {
        RemoveGameObjectHierarchy(childUUID);
    }

    uint32_t parentUUID = gameObject->GetParent();

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

void SceneModule::UpdateGameObjectHierarchy(uint32_t sourceUUID, uint32_t targetUUID)
{
    GameObject* sourceGameObject = GetGameObjectByUUID(sourceUUID);
    GameObject* targetGameObject = GetGameObjectByUUID(targetUUID);

    if (!sourceGameObject || !targetGameObject) return;

    uint32_t oldParentUUID = sourceGameObject->GetParent();
    sourceGameObject->SetParent(targetUUID);

    if (gameObjectsContainer.count(oldParentUUID))
    {
        GameObject* oldParentGameObject = GetGameObjectByUUID(oldParentUUID);
        oldParentGameObject->RemoveGameObject(sourceUUID);
    }

    targetGameObject->AddGameObject(sourceUUID);
}

AABBUpdatable* SceneModule::GetTargetForAABBUpdate(uint32_t uuid)
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

// SPATIAL_PARTITIONING TESTING
void SceneModule::CreateHouseGameObject(int totalGameobjects)
{
    math::LCG randomGenerator;

    for (int i = 0; i < totalGameobjects; ++i)
    {
        uint32_t newUUID          = LCG().IntFast();
        GameObject* newGameObject = new GameObject(gameObjectRootUUID, "House_" + std::to_string(i));
        newGameObject->SetUUID(newUUID);
        GetGameObjectByUUID(gameObjectRootUUID)->AddGameObject(newUUID);
        gameObjectsContainer.insert({newUUID, newGameObject});

        auto gameObjectChildrenUUID  = newGameObject->GetRootComponent()->GetUUID();
        ComponentType componentType  = ComponentType::COMPONENT_MESH;

        Component* selectedComponent = gameComponents[gameObjectChildrenUUID];
        uint32_t createdComponentUID = LCG().IntFast();
        Component* createdComponent  = ComponentUtils::CreateEmptyComponent(
            componentType, createdComponentUID, gameObjectChildrenUUID, gameObjectChildrenUUID,
            selectedComponent->GetGlobalTransform()
        );

        gameComponents[createdComponent->GetUUID()] = createdComponent;
        selectedComponent->AddChildComponent(createdComponent->GetUUID());

        MeshComponent* createdMesh = reinterpret_cast<MeshComponent*>(createdComponent);
        auto meshIT                = MOCKUP_libraryMeshes.begin();
        createdMesh->LoadMesh(meshIT->first, meshIT->second);

        auto textureIT = MOCKUP_libraryTextures.begin();
        createdMesh->SetTexture(textureIT->first, textureIT->second);

        float xPosition = randomGenerator.Float(-50, 50);
        float yPosition = randomGenerator.Float(-50, 50);
        float zPosition = randomGenerator.Float(-50, 50);

        float yRotation = randomGenerator.Float(-180, 180);

        Transform newTransform =
            Transform(float3(xPosition, yPosition, zPosition), float3(0, yRotation, 0), float3(100, 100, 100));

        newGameObject->AddGameObject(createdComponentUID);

        selectedComponent->OnTransformUpdate(newTransform);
    }
}

// SPATIAL_PARTITIONING TESTING
void SceneModule::RenderBoundingBoxes()
{
    std::vector<LineSegment> elementLines = std::vector<LineSegment>(gameObjectsContainer.size() * 12, LineSegment());
    int currentDrawLine                   = 0;

    for (auto& gameObject : gameObjectsContainer)
    {
        // AABB gameObjectBB = gameObject.second->GetGlobalBoundingBox();
        OBB objectOBB = gameObject.second->GetGlobalOrientedBoundingBox();

        for (int i = 0; i < 12; ++i)
        {
            elementLines[currentDrawLine++] = objectOBB.Edge(i);
        }
    }

    float4x4 view = App->GetCameraModule()->GetViewMatrix();
    float4x4 proj = App->GetCameraModule()->GetProjectionMatrix();
    int width     = App->GetOpenGLModule()->GetFramebuffer()->GetTextureWidth();
    int height    = App->GetOpenGLModule()->GetFramebuffer()->GetTextureHeight();

    App->GetDebugDrawModule()->RenderLines(elementLines, float3(1.f, 0.f, 0.f));

    App->GetDebugDrawModule()->Draw(view, proj, width, height);
}

void SceneModule::RenderOctree()
{
    std::vector<LineSegment> octreeLines;
    std::vector<LineSegment> gameObjectLines;

    sceneOctree->GetDrawLines(octreeLines, gameObjectLines);

    App->GetDebugDrawModule()->RenderLines(octreeLines, float3(1.f, 0.f, 0.f));

    float4x4 view = App->GetCameraModule()->GetViewMatrix();
    float4x4 proj = App->GetCameraModule()->GetProjectionMatrix();
    int width     = App->GetOpenGLModule()->GetFramebuffer()->GetTextureWidth();
    int height    = App->GetOpenGLModule()->GetFramebuffer()->GetTextureHeight();

    App->GetDebugDrawModule()->Draw(view, proj, width, height);
}
