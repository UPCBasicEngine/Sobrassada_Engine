#include "Scene.h"

#include "Application.h"
#include "BatchManager.h"
#include "BillboardModule.h"
#include "CameraComponent.h"
#include "CameraModule.h"
#include "Component.h"
#include "ComponentUtils.h"
#include "Components/ShaderScriptComponent.h"
#include "DebugDrawModule.h"
#include "EditorUIModule.h"
#include "Framebuffer.h"
#include "GBuffer.h"
#include "GameObject.h"
#include "GameTimer.h"
#include "GeometryBatch.h"
#include "Importer.h"
#include "InputModule.h"
#include "LibraryModule.h"
#include "ModelImporter.h"
#include "Octree.h"
#include "OpenGLModule.h"
#include "ParticleSystemComponent.h"
#include "ParticleSystemModule.h"
#include "PathfinderModule.h"
#include "PhysicsModule.h"
#include "ProjectModule.h"
#include "Quadtree.h"
#include "RenderPass.h"
#include "Resource.h"
#include "ResourceMaterial.h"
#include "ResourceModel.h"
#include "ResourcePrefab.h"
#include "ResourcesModule.h"
#include "SSAO.h"
#include "SceneModule.h"
#include "ScriptComponent.h"
#include "ShaderModule.h"
#include "ShaderScriptModule.h"
#include "Standalone/AIAgentComponent.h"
#include "Standalone/AnimationComponent.h"
#include "Standalone/Audio/AudioListenerComponent.h"
#include "Standalone/Audio/AudioSourceComponent.h"
#include "Standalone/BillboardComponent.h"
#include "Standalone/CharacterControllerComponent.h"
#include "Standalone/DecalComponent.h"
#include "Standalone/Lights/DirectionalLightComponent.h"
#include "Standalone/Lights/PointLightComponent.h"
#include "Standalone/Lights/SpotLightComponent.h"
#include "Standalone/MeshComponent.h"
#include "Standalone/Physics/CapsuleColliderComponent.h"
#include "Standalone/Physics/CubeColliderComponent.h"
#include "Standalone/Physics/SphereColliderComponent.h"
#include "Standalone/SplineComponent.h"
#include "Standalone/TrailComponent.h"
#include "Standalone/UI/ButtonComponent.h"
#include "Standalone/UI/CanvasComponent.h"
#include "Standalone/UI/CanvasScalerComponent.h"
#include "Standalone/UI/ImageComponent.h"
#include "Standalone/UI/Transform2DComponent.h"
#include "Standalone/UI/UILabelComponent.h"
#include "Standalone/VideoComponent.h"

#include "SDL_mouse.h"
#include "glew.h"
#include "imgui.h"
#include "imgui_internal.h"
// guizmo after imgui include
#include "ImGuizmo.h"
#ifdef OPTICK
#include "optick.h"
#endif

#include "WindConfig.h"

#include <set>
#include <unordered_set>
#include <unordered_map>

Scene::Scene(const char* sceneName) : sceneUID(GenerateUID())
{
    this->sceneName             = sceneName;

    GameObject* sceneGameObject = new GameObject("SceneModule GameObject");
    selectedGameObjectUID = gameObjectRootUID = sceneGameObject->GetUID();

    gameObjectsContainer.insert({sceneGameObject->GetUID(), sceneGameObject});

    lightsConfig = new LightsConfig();
    windConfig = new WindConfig();
    renderPass   = new RenderPass();
}

Scene::Scene(const rapidjson::Value& initialState, UID loadedSceneUID) : sceneUID(loadedSceneUID)
{
    this->sceneName       = initialState["Name"].GetString();
    gameObjectRootUID     = initialState["RootGameObject"].GetUint64();
    selectedGameObjectUID = gameObjectRootUID;
    if (initialState.HasMember("NavmeshUID")) navmeshUID = initialState["NavmeshUID"].GetUint64();

    App->GetCameraModule()->LoadCameraPosition(&initialState);

    App->GetPhysicsModule()->LoadLayerData(&initialState);

    // Load navmesh from scene.
    if (navmeshUID != INVALID_UID)
    {
        std::string navmeshName = App->GetLibraryModule()->GetResourceName(navmeshUID);
        App->GetPathfinderModule()->LoadNavMesh(navmeshName);
    }

    // Deserialize GameObjects
    if (initialState.HasMember("GameObjects") && initialState["GameObjects"].IsArray())
    {
        // Create GameObjects
        const rapidjson::Value& gameObjects = initialState["GameObjects"];

        for (rapidjson::SizeType i = 0; i < gameObjects.Size(); i++)
        {
            const rapidjson::Value& gameObject = gameObjects[i];

            GameObject* newGameObject          = new GameObject(gameObject);
            if (newGameObject->GetPrefabUID() != INVALID_UID)
                LoadNestedPrefab(newGameObject);
            else
            {
                gameObjectsContainer.insert({newGameObject->GetUID(), newGameObject});
                gameObjectDataMap[newGameObject->GetUID()] = &gameObject;
            }
        }
    }

    // Deserialize Lights Config
    if (initialState.HasMember("Lights Config") && initialState["Lights Config"].IsObject())
    {
        lightsConfig = new LightsConfig();
        lightsConfig->LoadData(initialState["Lights Config"]);
    }

    // Deserialize Wind Config
    windConfig = new WindConfig();
    if (initialState.HasMember("Wind Config") && initialState["Wind Config"].IsObject())
    {
        windConfig->LoadData(initialState["Wind Config"]);
    }

    renderPass = new RenderPass();

    if (initialState.HasMember("tags") && initialState.HasMember("tagsGO"))
    {
        const rapidjson::Value& tagDataArray   = initialState["tags"];
        const rapidjson::Value& tagGODataArray = initialState["tagsGO"];

        for (rapidjson::SizeType i = 0; i < tagDataArray.Size(); i++)
        {
            HashString newTag = HashString(tagDataArray[i].GetString());
            CreateTag(std::move(newTag));

            for (int j = 0; j < tagGODataArray[i].Size(); ++j)
            {
                RequestTag(
                    HashString(tagDataArray[i].GetString()), GetGameObjectByUID(tagGODataArray[i][j].GetUint64())
                );
            }
        }
    }

    // GLOG("%s scene loaded", sceneName.c_str());
}

Scene::~Scene()
{
    for (auto it = gameObjectsContainer.begin(); it != gameObjectsContainer.end(); ++it)
    {
        delete it->second;
    }

    App->GetPhysicsModule()->EmptyWorld();

    gameObjectsContainer.clear();

    selectedGameObjects.clear();

    App->GetParticleModule()->ClearParticleSystems();

    App->GetPathfinderModule()->ClearNavMesh();
    delete lightsConfig;
    delete windConfig;
    delete sceneOctree;
    delete dynamicTree;
    delete renderPass;

    lightsConfig = nullptr;
    windConfig = nullptr;
    sceneOctree  = nullptr;
    dynamicTree  = nullptr;

    // GLOG("%s scene closed", sceneName.c_str());
}

void Scene::Init()
{
    multiSelectParent = new GameObject(GenerateUID(), "MULTISELECT_DUMMY");
    gameObjectsContainer.insert({multiSelectParent->GetUID(), multiSelectParent});

    // Init data
    for (const auto& pair : gameObjectDataMap)
    {
        GameObject* gameObjectToLoad = GetGameObjectByUID(pair.first);
        gameObjectToLoad->LoadData(*pair.second);
    }
    gameObjectDataMap.clear();
    // When loading a scene, overrides all gameObjects that have a prefabUID. That is because if the prefab has been
    // modified, the scene file may have not, so the prefabs need to be updated when loading the scene again
    std::vector<UID> prefabs;
    for (const auto& gameObject : gameObjectsContainer)
    {
        if (gameObject.second->GetPrefabUID() == INVALID_UID) continue;

        // Add to prefabs UIDs if not existing, only once each
        std::vector<UID>::iterator it = std::find(prefabs.begin(), prefabs.end(), gameObject.second->GetPrefabUID());
        if (it == prefabs.end()) prefabs.emplace_back(gameObject.second->GetPrefabUID());
    }

    lightsConfig->InitSkybox();
    lightsConfig->InitLightBuffers();

    for (const UID prefab : prefabs)
    {
        DeletePrefab(prefab);
    }

    for (auto& gameObject : gameObjectsContainer)
    {
        if (gameObject.second->GetParent() == gameObjectRootUID) gameObject.second->InitHierarchy();
    }

    App->GetResourcesModule()->GetBatchManager()->LoadData();

    // Initialize the skinning for all the gameObjects that need it
    for (const auto& gameObject : gameObjectsContainer)
    {
        MeshComponent* mesh = gameObject.second->GetComponent<MeshComponent*>();
        if (mesh) mesh->InitSkin();
    }

    // Call this after overriding the prefabs to avoid duplicates in gameObjectsToUpdateComponents
    GetGameObjectByUID(gameObjectRootUID)->UpdateTransformForGOBranch();

    // Worst case all objects are in frustum
    toUpdateGameObjects.reserve(gameObjectsContainer.size());

    UpdateStaticSpatialStructure();
    UpdateDynamicSpatialStructure();

    isSceneLoaded = true;
}

void Scene::Save(
    rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator, SaveMode saveMode, UID newUID,
    const char* newName
)
{
    if (newUID != INVALID_UID)
    {
        sceneUID = newUID;
    }
    if (newName != nullptr)
    {
        sceneName = newName;
    }

    targetState.AddMember("UID", sceneUID, allocator);
    targetState.AddMember("Name", rapidjson::Value(sceneName.c_str(), allocator), allocator);

    targetState.AddMember("RootGameObject", gameObjectRootUID, allocator);
    targetState.AddMember("NavmeshUID", navmeshUID, allocator);

    App->GetCameraModule()->SaveCameraPosition(targetState, allocator);

    App->GetPhysicsModule()->SaveLayerData(targetState, allocator);

    // SAVING FIRST
    rapidjson::Value sceneTagsJSON(rapidjson::kArrayType);
    rapidjson::Value sceneTagsGameObjectsJSON(rapidjson::kArrayType);
    for (auto& tagPair : tags)
    {
        rapidjson::Value currentTagGO(rapidjson::kArrayType);
        for (GameObject* currentGO : tagPair.second)
        {
            currentTagGO.PushBack(currentGO->GetUID(), allocator);
        }
        sceneTagsJSON.PushBack(rapidjson::Value(tagPair.first.c_str(), allocator), allocator);
        sceneTagsGameObjectsJSON.PushBack(currentTagGO, allocator);
    }
    targetState.AddMember("tags", sceneTagsJSON, allocator);
    targetState.AddMember("tagsGO", sceneTagsGameObjectsJSON, allocator);

    // Serialize GameObjects
    rapidjson::Value gameObjectsJSON(rapidjson::kArrayType);

    //if (!IsGloballyEnabled()) return;
    std::stack<UID> childrenBuffer;
    childrenBuffer.push(gameObjectRootUID);

    while (!childrenBuffer.empty())
    {
        GameObject* gameObject = GetGameObjectByUID(childrenBuffer.top());
        childrenBuffer.pop();
        if (gameObject != nullptr && gameObject != multiSelectParent)
        {
            rapidjson::Value goJSON(rapidjson::kObjectType);

            gameObject->Save(goJSON, allocator);

            gameObjectsJSON.PushBack(goJSON, allocator);

            if (gameObject->GetPrefabUID() == INVALID_UID)
            {
                for (UID child : gameObject->GetChildren())
                    childrenBuffer.push(child);
            }
        }
    }

    // Add gameObjects to scene
    targetState.AddMember("GameObjects", gameObjectsJSON, allocator);

    // Serialize Lights Config
    LightsConfig* lightConfig = App->GetSceneModule()->GetScene()->GetLightsConfig();

    if (lightConfig != nullptr)
    {
        rapidjson::Value lights(rapidjson::kObjectType);

        lightConfig->SaveData(lights, allocator);

        targetState.AddMember("Lights Config", lights, allocator);
    }

    else GLOG("Light Config not found");

    // Serialize Wind Config
    rapidjson::Value wind(rapidjson::kObjectType);
    windConfig->SaveData(wind, allocator);
    targetState.AddMember("Wind Config", wind, allocator);

    // TODO Convert to parameter which can be set later manually instead of saving a scene as default "on scene
    // save"
    if (saveMode != SaveMode::SavePlayMode) App->GetProjectModule()->SetAsStartupScene(sceneName);
}

update_status Scene::Update(float deltaTime)
{
#ifdef OPTICK
    OPTICK_CATEGORY("Scene::Update", Optick::Category::GameLogic)
#endif
    if (App->GetSceneModule()->GetOnlyOnceInPlayMode())
    {
        for (auto& gameObject : gameObjectsContainer)
        {
            ScriptComponent* script = gameObject.second->GetComponent<ScriptComponent*>();
            if (script) script->InitScriptInstances();

            ShaderScriptComponent* shaderScript = gameObject.second->GetComponent<ShaderScriptComponent*>();
            if (shaderScript) shaderScript->InitScriptInstances();
        }
        App->GetSceneModule()->ResetOnlyOnceInPlayMode();
    }

    for (auto gameObject : toUpdateGameObjects)
        gameObject->UpdateComponents(deltaTime);

    ImGuiWindow* window = ImGui::FindWindowByName(sceneName.c_str());
    if (window && !(window->Hidden || window->Collapsed)) sceneVisible = true;
    else sceneVisible = false;

    return UPDATE_CONTINUE;
}

update_status Scene::Render(float deltaTime)
{
    CameraComponent* mainCamera = App->GetSceneModule()->GetScene()->GetMainCamera();
    if (App->GetSceneModule()->GetInPlayMode() && mainCamera != nullptr)
    {
        if (mainCamera->IsEffectivelyEnabled()) RenderScene(deltaTime, mainCamera);
        else RenderScene(deltaTime, nullptr);
    }
    else RenderScene(deltaTime, nullptr);

    GameObject* selectedGameObject = App->GetSceneModule()->GetScene()->GetSelectedGameObject();
    if (selectedGameObject != nullptr) selectedGameObject->RenderDebugComponents(deltaTime);

    return UPDATE_CONTINUE;
}

void Scene::RenderScene(float deltaTime, CameraComponent* camera)
{
    GBuffer* gbuffer         = App->GetOpenGLModule()->GetGBuffer();
    SSAO* ssao               = App->GetOpenGLModule()->GetSsao();
    Framebuffer* framebuffer = App->GetSceneModule()->GetInPlayMode() ? App->GetOpenGLModule()->GetFramebuffer()
                             : camera != nullptr                      ? camera->GetFramebuffer()
                                                                      : App->GetOpenGLModule()->GetFramebuffer();

#ifdef OPTICK
    OPTICK_CATEGORY("Scene::MeshesToRender", Optick::Category::GameLogic)
#endif

    renderPass->RenderScene(framebuffer, toUpdateGameObjects, camera, deltaTime);

#ifndef GAME
    for (const auto& gameObject : toUpdateGameObjects)
    {
        gameObject->DrawGizmos();
    }
#endif

#ifdef OPTICK
    OPTICK_CATEGORY("Scene::GameObject::Render_DebugDraw", Optick::Category::Rendering)
#endif
    DebugDrawModule* debugDraw = App->GetDebugDrawModule();

    for (auto& gameObjectIterator : selectedGameObjects)
    {
        GameObject* gameObject = GetGameObjectByUID(gameObjectIterator.first);

        const AABB aabb        = gameObject->GetHierarchyAABB();

        for (int i = 0; i < 12; ++i)
            debugDraw->DrawLineSegment(aabb.Edge(i), float3(1.f, 1.0f, 0.5f));
    }
}

update_status Scene::RenderEditor(float deltaTime)
{
    EditorUIModule* editor = App->GetEditorUIModule();
    if (editor->editorControlMenu) RenderEditorControl(editor->editorControlMenu);

    RenderSceneToFrameBuffer();

    RenderSelectedGameObjectUI();
    if (editor->lightConfig) lightsConfig->EditorParams(editor->lightConfig);

    return UPDATE_CONTINUE;
}

void Scene::RenderEditorControl(bool& editorControlMenu)
{
    if (!ImGui::Begin("Editor Control", &editorControlMenu))
    {
        ImGui::End();
        return;
    }

    GizmoOperation& currentGizmoOperation = App->GetEditorUIModule()->GetCurrentGizmoOperation();
    int selectedOp                        = static_cast<int>(currentGizmoOperation);
    ImGui::PushItemWidth(150);
    ImGui::RadioButton("T", &selectedOp, 0);
    ImGui::SameLine();
    ImGui::RadioButton("R", &selectedOp, 1);
    ImGui::SameLine();
    ImGui::RadioButton("S", &selectedOp, 2);
    ImGui::PopItemWidth();

    if (selectedOp == 0) currentGizmoOperation = GizmoOperation::TRANSLATE;
    else if (selectedOp == 1) currentGizmoOperation = GizmoOperation::ROTATE;
    else if (selectedOp == 2) currentGizmoOperation = GizmoOperation::SCALE;

    ImGui::SameLine();
    ImGui::Text("|");
    ImGui::SameLine();

    GizmoTransform& transformType = App->GetEditorUIModule()->GetTransformType();
    int selectedMode              = static_cast<int>(transformType);
    ImGui::PushItemWidth(100);
    ImGui::RadioButton("L", &selectedMode, 0);
    ImGui::SameLine();
    ImGui::RadioButton("W", &selectedMode, 1);
    ImGui::PopItemWidth();

    if (selectedMode == 0) transformType = GizmoTransform::LOCAL;
    else if (selectedMode == 1) transformType = GizmoTransform::WORLD;

    ImGui::SameLine();
    ImGui::Text("|");
    ImGui::SameLine();

    float3& snapValues = App->GetEditorUIModule()->GetSnapValues();
    ImGui::PushItemWidth(150);
    ImGui::Text("Snap");
    ImGui::SameLine();
    ImGui::Checkbox("##snapEnabled", &App->GetEditorUIModule()->snapEnabled);
    ImGui::SameLine();
    ImGui::InputFloat3("##snap", &snapValues.x);
    ImGui::PopItemWidth();

    ImGui::SameLine();
    ImGui::Text("|");
    ImGui::SameLine();

    GameTimer* gameTimer = App->GetGameTimer();

    float timeScale      = gameTimer->GetTimeScale();

    if (ImGui::Button("Play"))
    {
        startPlaying = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Pause"))
    {
        if (App->GetSceneModule()->GetInPlayMode()) gameTimer->TogglePause();
    }
    ImGui::SameLine();
    if (ImGui::Button("Step"))
    {
        if (App->GetSceneModule()->GetInPlayMode()) stepPlaying = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Stop"))
    {
        if (App->GetSceneModule()->GetInPlayMode()) stopPlaying = true;
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100.0f);
    if (ImGui::SliderFloat("Time scale", &timeScale, 0, 4)) gameTimer->SetTimeScale(timeScale);

    // RENDER OPTIONS
    if (ImGui::Button("Render options") || App->GetInputModule()->GetKeyboard()[SDL_SCANCODE_F9])
    {
        ImGui::OpenPopup("RenderOptions");
    }

    if (ImGui::BeginPopup("RenderOptions"))
    {
        int stringCount   = sizeof(DebugStrings) / sizeof(char*);
        float listBoxSize = (float)stringCount + 0.5f;
        if (ImGui::BeginListBox(
                "##RenderOptionsList", ImVec2(ImGui::CalcItemWidth(), ImGui::GetFrameHeightWithSpacing() * listBoxSize)
            ))
        {
            const auto& debugBitset = App->GetDebugDrawModule()->GetDebugOptionValues();
            for (int i = 0; i < stringCount; ++i)
            {
                bool currentBitValue = debugBitset[i];
                if (ImGui::Checkbox(DebugStrings[i], &currentBitValue))
                {
                    App->GetDebugDrawModule()->FlipDebugOptionValue(i);
                    if (i == (int)DebugOptions::RENDER_PHYSICS_WORLD)
                        App->GetPhysicsModule()->SetDebugOption(currentBitValue);
                }
            }

            ImGui::EndListBox();
        }

        ImGui::EndPopup();
    }
    if (App->GetSceneModule()->GetInPlayMode())
    {
        ImGui::SeparatorText("Playing");
        ImGui::Text("Frame count: %d", gameTimer->GetFrameCount());
        ImGui::SameLine();
        ImGui::Text("Game time: %.3f", gameTimer->GetTime() / 1000.0f);
        ImGui::SameLine();
        ImGui::Text("Delta time: %.3f", gameTimer->GetDeltaTime() / 1000.0f);
        // ImGui::Text("Unscaled game time: %.3f", gameTimer->GetUnscaledTime() / 1000.0f);
        // ImGui::Text("Unscaled delta time: %.3f", gameTimer->GetUnscaledDeltaTime() / 1000.0f);
        // ImGui::Text("Reference time: %.3f", gameTimer->GetReferenceTime() / 1000.0f);
    }
    ImGui::End();
}

void Scene::RenderSceneToFrameBuffer()
{
    if (!ImGui::Begin(sceneName.c_str(), nullptr, ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar))
    {
        ImGui::End();
        return;
    }

    // right click focus window
    if (ImGui::IsWindowHovered() &&
        (ImGui::IsMouseClicked(ImGuiMouseButton_Right) || ImGui::IsMouseClicked(ImGuiMouseButton_Middle)))
        ImGui::SetWindowFocus();

    isFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_DockHierarchy);

    // do inputs only if window is focused
    if (ImGui::IsWindowHovered(ImGuiFocusedFlags_DockHierarchy))
    {
        doMouseInputs = true;
        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_DockHierarchy)) doInputs = true;
    }
    else
    {
        doInputs      = false;
        doMouseInputs = false;
    }

    const auto& framebuffer = App->GetOpenGLModule()->GetFramebuffer();

    ImGui::SetCursorPos(ImVec2(0.f, 0.f));

    ImGui::Image(
        (ImTextureID)framebuffer->GetTextureID(),
        ImVec2((float)framebuffer->GetTextureWidth(), (float)framebuffer->GetTextureHeight()), ImVec2(0.f, 1.f),
        ImVec2(1.f, 0.f)
    );

    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetDrawlist(); // ImGui::GetWindowDrawList()

    float width  = ImGui::GetWindowWidth();
    float height = ImGui::GetWindowHeight();
    ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, width, height);

    ImVec2 windowSize = ImGui::GetWindowSize();
    if (framebuffer->GetTextureWidth() != windowSize.x || framebuffer->GetTextureHeight() != windowSize.y)
    {
        float aspectRatio = windowSize.y / windowSize.x;
        if (App->GetSceneModule()->GetScene()->GetMainCamera() != nullptr)
            App->GetSceneModule()->GetScene()->GetMainCamera()->SetAspectRatio(aspectRatio);
        App->GetCameraModule()->SetAspectRatio(aspectRatio);
        framebuffer->Resize((int)windowSize.x, (int)windowSize.y);
        App->GetOpenGLModule()->GetGBuffer()->Resize((int)windowSize.x, (int)windowSize.y);
        App->GetOpenGLModule()->GetSsao()->Resize((int)windowSize.x, (int)windowSize.y);
    }

    ImVec2 windowPosition     = ImGui::GetWindowPos();
    ImVec2 imGuimousePosition = ImGui::GetMousePos();
    sceneWindowPosition       = std::make_tuple(windowPosition.x, windowPosition.y);
    sceneWindowSize           = std::make_tuple(windowSize.x, windowSize.y);
    mousePosition             = std::make_tuple(imGuimousePosition.x, imGuimousePosition.y);

    ImGui::End();
}

void Scene::RenderSelectedGameObjectUI()
{
    GameObject* selectedGameObject = GetSelectedGameObject();
    if (selectedGameObject != nullptr)
    {
        selectedGameObject->RenderEditor();
    }
}

void Scene::RenderHierarchyUI(bool& hierarchyMenu)
{
    if (!ImGui::Begin("Hierarchy", &hierarchyMenu))
    {
        ImGui::End();
        return;
    }

    if (ImGui::Button("Add GameObject"))
    {

        GameObject* parent = GetGameObjectByUID(selectedGameObjectUID);
        if (parent != nullptr)
        {
            GameObject* newGameObject = new GameObject(selectedGameObjectUID, "new Game Object");

            gameObjectsContainer.insert({newGameObject->GetUID(), newGameObject});
            parent->AddGameObject(newGameObject->GetUID());

            newGameObject->UpdateTransformForGOBranch();
        }
    }

    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
    {
        ImGui::OpenPopup("HierarchyContextMenu");
    }

    if (ImGui::BeginPopup("HierarchyContextMenu"))
    {
        if (ImGui::MenuItem("Add GameObject"))
        {
            GameObject* parent = GetGameObjectByUID(gameObjectRootUID);

            if (parent != nullptr)
            {
                GameObject* newGameObject = new GameObject(gameObjectRootUID, "new Game Object");

                gameObjectsContainer.insert({newGameObject->GetUID(), newGameObject});
                parent->AddGameObject(newGameObject->GetUID());

                newGameObject->UpdateTransformForGOBranch();
            }
        }

        ImGui::EndPopup();
    }

    if (selectedGameObjectUID != gameObjectRootUID)
    {
        ImGui::SameLine();

        if (ImGui::Button("Delete GameObject"))
        {
            RemoveGameObjectHierarchy(selectedGameObjectUID);
        }
    }

    GameObject* rootGameObject = GetGameObjectByUID(gameObjectRootUID);
    if (rootGameObject)
    {
        rootGameObject->RenderHierarchyNode(selectedGameObjectUID);
    }

    ImGui::End();
}

void Scene::RemoveGameObjectHierarchy(UID gameObjectUID)
{
    // TODO: Change when filesystem defined
    if (!gameObjectsContainer.count(gameObjectUID) || gameObjectUID == gameObjectRootUID ||
        (multiSelectParent && gameObjectUID == multiSelectParent->GetUID()))
        return;

    std::stack<UID> toDelete;
    toDelete.push(gameObjectUID);
    GameObject* gameObject = GetGameObjectByUID(gameObjectUID);

    //
    if (gameObject->IsStatic()) SetStaticModified();
    else SetDynamicModified();

    std::vector<UID> collectedUIDs;

    // Collect all UIDs to delete
    while (!toDelete.empty())
    {
        UID currentUID = toDelete.top();
        toDelete.pop();

        GameObject* gameObject = GetGameObjectByUID(currentUID);

        if (gameObject->IsStatic()) SetStaticModified();
        else SetDynamicModified();

        if (gameObject == nullptr) continue;

        collectedUIDs.push_back(currentUID);

        for (UID childUID : gameObject->GetChildren())
        {
            toDelete.push(childUID);
        }
    }

    // Remove from parent
    UID parentUID = GetGameObjectByUID(gameObjectUID)->GetParent();
    if (gameObjectsContainer.count(parentUID))
    {
        GameObject* parentGameObject = GetGameObjectByUID(parentUID);
        parentGameObject->RemoveGameObject(gameObjectUID);
        selectedGameObjectUID = parentUID;
    }

    // Delete collected game objects
    for (UID uid : collectedUIDs)
    {
        if (GetGameObjectByUID(uid)->IsStatic()) SetStaticModified();
        else SetDynamicModified();

        delete gameObjectsContainer[uid];
        gameObjectsContainer.erase(uid);
    }
}

void Scene::AddGameObjectToUpdateComponents(GameObject* gameObject)
{
    if (!gameObject->WillUpdate())
    {
        gameObject->SetWillUpdate(true);
        gameObjectsToUpdateComponents.push_back(gameObject);
    }
}

void Scene::UpdateGameObjectsComponents()
{
    for (GameObject* gameObject : gameObjectsToUpdateComponents)
    {
        if (gameObject)
        {
            gameObject->ParentUpdatedComponents();
            gameObject->SetWillUpdate(false);
        }
    }
    gameObjectsToUpdateComponents.clear();
}

void Scene::ClearGameObjectsToUpdateComponents()
{
    gameObjectsToUpdateComponents.clear();
}

void Scene::AddGameObjectToSelection(UID gameObject, UID gameObjectParent)
{
    GameObject* selectedGameObject = GetGameObjectByUID(gameObject);
    auto pairResult                = selectedGameObjects.insert({gameObject, gameObjectParent});

    MobilitySettings gameObjectMobility =
        selectedGameObject->IsStatic() ? MobilitySettings::STATIC : MobilitySettings::DYNAMIC;

    auto pairResultMobility = selectedGameObjectsMobility.insert({gameObject, gameObjectMobility});
    auto pairResultLocals   = selectedGameObjectsOgLocals.insert({gameObject, selectedGameObject->GetLocalTransform()});

    if (pairResult.second)
    {
        GameObject* selectedGameObjectParent = GetGameObjectByUID(gameObjectParent);

        // selectedGameObjectParent->RemoveGameObject(gameObject);

        multiSelectParent->AddGameObject(gameObject);

        selectedGameObject->SetParent(multiSelectParent->GetUID());
        selectedGameObject->UpdateLocalTransform(multiSelectParent->GetGlobalTransform());
        selectedGameObject->UpdateTransformForGOBranch();

        selectedGameObjectUID = multiSelectParent->GetUID();
    }
    else if (pairResult.first != selectedGameObjects.end())
    {
        multiSelectParent->RemoveGameObject(gameObject);

        GameObject* selectedGameObjectParent = GetGameObjectByUID(selectedGameObjects[gameObject]);

        selectedGameObject->SetParent(selectedGameObjectParent->GetUID());

        if (selectedGameObjectParent->GetUID() != gameObjectRootUID)
        {
            selectedGameObject->UpdateLocalTransform(selectedGameObjectParent->GetGlobalTransform());
            selectedGameObject->UpdateTransformForGOBranch();
        }

        selectedGameObjects.erase(pairResult.first);
        selectedGameObjectsMobility.erase(pairResultMobility.first);
        selectedGameObjectsOgLocals.erase(pairResultLocals.first);
    }
}

void Scene::ClearObjectSelection()
{
    for (auto& pairGameObject : selectedGameObjects)
    {
        GameObject* currentGameObject        = GetGameObjectByUID(pairGameObject.first);
        GameObject* selectedGameObjectParent = GetGameObjectByUID(pairGameObject.second);

        multiSelectParent->RemoveGameObject(pairGameObject.first);

        currentGameObject->SetParent(pairGameObject.second);
        selectedGameObjectParent->AddGameObject(pairGameObject.first);

        currentGameObject->UpdateLocalTransform(selectedGameObjectParent->GetGlobalTransform());
        currentGameObject->UpdateTransformForGOBranch();
    }

    // UPDATE TO LET ORIGINAL GAME OBJECTS WITH THEIR ORIGINAL MOBILITY
    for (auto& pairGameObject : selectedGameObjectsMobility)
    {
        GameObject* currentGameObject = GetGameObjectByUID(pairGameObject.first);
        currentGameObject->UpdateMobilityHierarchy(pairGameObject.second);
    }

    selectedGameObjects.clear();
    selectedGameObjectsMobility.clear();
    selectedGameObjectsOgLocals.clear();
}

void Scene::DeleteMultiselection()
{
    for (auto& pairGameObject : selectedGameObjects)
    {
        GameObject* currentGameObject        = GetGameObjectByUID(pairGameObject.first);
        GameObject* selectedGameObjectParent = GetGameObjectByUID(pairGameObject.second);

        multiSelectParent->RemoveGameObject(pairGameObject.first);

        selectedGameObjectParent->RemoveGameObject(pairGameObject.first);
        selectedGameObjectParent->UpdateTransformForGOBranch();

        RemoveGameObjectHierarchy(pairGameObject.first);
    }
    selectedGameObjects.clear();
    selectedGameObjectsMobility.clear();
    ClearGameObjectsToUpdateComponents();
}

void Scene::CreateTag(HashString&& newTag)
{
    if (newTag != emptyString && tags.find(newTag) == tags.end())
    {
        tags.insert({std::move(newTag), {}});
    }
}

void Scene::DeleteTag(const HashString& tagToDelete)
{
    auto tagIterator = tags.find(tagToDelete);
    if (tagIterator != tags.end())
    {
        for (GameObject* gameObject : tagIterator->second)
        {
            gameObject->RemoveTag(tagToDelete);
        }

        tags.erase(tagIterator);
    }
}

void Scene::RequestTag(const HashString& requestTag, GameObject* gameObject)
{
    if (!gameObject) return;
    auto tagIterator = tags.find(requestTag);
    if (tagIterator != tags.end() && !gameObject->HasTag(requestTag))
    {
        tags[requestTag].push_back(gameObject);
        gameObject->AddTag(requestTag);
    }
}

void Scene::RemoveFromTag(const HashString& requestTag, GameObject* gameObject)
{
    if (!gameObject) return;
    auto tagIterator = tags.find(requestTag);
    if (tagIterator != tags.end() && gameObject->HasTag(requestTag))
    {
        std::vector<GameObject*>& gameObjects = tags[requestTag];

        for (auto goIterator = gameObjects.begin(); goIterator != gameObjects.end(); ++goIterator)
        {
            if (*goIterator == gameObject)
            {
                gameObject->RemoveTag(requestTag);
                gameObjects.erase(goIterator);
                return;
            }
        }
    }
}

const std::vector<GameObject*>* Scene::GetTaggedGameObjects(const HashString& requestTag)
{
    if (tags.find(requestTag) != tags.end())
    {
        return &tags[requestTag];
    }

    return nullptr;
}

UID Scene::GetMultiselectUID() const
{
    return multiSelectParent->GetUID();
}

void Scene::SetMultiselectPosition(const float3& newPosition)
{
    const float4x4 localMat = float4x4::FromTRS(newPosition, float4x4::identity, float3::one);
    multiSelectParent->SetLocalTransform(localMat);
}

void Scene::CheckObjectsToUpdate()
{
    CameraComponent* mainCamera = App->GetSceneModule()->GetScene()->GetMainCamera();
    if (App->GetSceneModule()->GetInPlayMode() && mainCamera != nullptr)
        CheckObjectsInFrustum(toUpdateGameObjects, mainCamera->GetFrustrumPlanes());

    else CheckObjectsInFrustum(toUpdateGameObjects, App->GetCameraModule()->GetFrustrumPlanes());

    for (auto gameObject : toUpdateGameObjects)
        toUpdateGameObjectsSet.insert(gameObject->GetUID());

    // ADD ALWAYS UPDATE TAG - UPDATE
    auto alwaysUpdateObjects = GetTaggedGameObjects(HashString("UPDATE"));
    if (alwaysUpdateObjects)
    {
        for (GameObject* currentGameObject : *alwaysUpdateObjects)
        {
            if (toUpdateGameObjectsSet.find(currentGameObject->GetUID()) == toUpdateGameObjectsSet.end())
            {
                toUpdateGameObjects.push_back(currentGameObject);
                toUpdateGameObjectsSet.insert(currentGameObject->GetUID());
            }
        }
    }

    // ADDING OBJECTS ASSIGNED TO LOCATION
    auto taggedLocationObjects = GetTaggedGameObjects(playerLocation);
    if (taggedLocationObjects)
    {
        for (GameObject* currentGameObject : *taggedLocationObjects)
        {
            if (toUpdateGameObjectsSet.find(currentGameObject->GetUID()) == toUpdateGameObjectsSet.end())
            {
                toUpdateGameObjects.push_back(currentGameObject);
                toUpdateGameObjectsSet.insert(currentGameObject->GetUID());
            }
        }
    }

    // ADDING CAMERA MANUALLY BECAUSE ITS NOT INSIDE ITS OWN FRUSTUM
    GameObject* camera = GetGameObjectByName("Camera");
    if (camera && toUpdateGameObjectsSet.find(camera->GetUID()) == toUpdateGameObjectsSet.end())
    {
        toUpdateGameObjects.push_back(camera);
        toUpdateGameObjectsSet.insert(camera->GetUID());
    }
    if (camera)
    {
        GameObject* cameraParent = GetGameObjectByUID(camera->GetParent());

        if (toUpdateGameObjectsSet.find(cameraParent->GetUID()) == toUpdateGameObjectsSet.end())
        {
            toUpdateGameObjects.push_back(cameraParent);
            toUpdateGameObjectsSet.insert(cameraParent->GetUID());
        }
    }
}

void Scene::ClearObjectsToUpdate()
{
    toUpdateGameObjectsSet.clear();
    toUpdateGameObjects.clear();
}

void Scene::UpdateAllMaterialInstances(const UID materialUID)
{
    for (const auto& object : gameObjectsContainer)
    {
        MeshComponent* mesh = object.second->GetComponent<MeshComponent*>();
        if (mesh && mesh->GetResourceMaterial()->GetUID() == materialUID)
        {
            mesh->BatchEditorMode();
        }
    }
}

void Scene::CreateStaticSpatialDataStruct()
{
    // PARAMETRIZED IN FUTURE
    float3 octreeCenter = float3::zero;
    float octreeLength  = 2000;
    int nodeCapacity    = 10;
    sceneOctree         = new Octree(octreeCenter, octreeLength, nodeCapacity);

    for (const auto& objectIterator : gameObjectsContainer)
    {
        AABB objectBB = objectIterator.second->GetGlobalAABB();

        if (!objectIterator.second->IsStatic()) continue;
        if (objectIterator.second->GetUID() == gameObjectRootUID) continue;
        if (!objectBB.IsFinite() || objectBB.IsDegenerate() || objectBB.Size().IsZero()) continue;

        sceneOctree->InsertElement(objectIterator.second);
    }
}

void Scene::CreateDynamicSpatialDataStruct()
{
    // PARAMETRIZED IN FUTURE
    float3 center    = float3::zero;
    float length     = 2000;
    int nodeCapacity = 15;
    dynamicTree      = new Octree(center, length, nodeCapacity);

    for (const auto& objectIterator : gameObjectsContainer)
    {
        AABB objectBB = objectIterator.second->GetGlobalAABB();

        if (objectIterator.second->IsStatic()) continue;
        if (objectIterator.second->GetUID() == gameObjectRootUID) continue;
        if (!objectBB.IsFinite() || objectBB.IsDegenerate() || objectBB.Size().IsZero()) continue;

        dynamicTree->InsertElement(objectIterator.second);
    }
}

void Scene::UpdateStaticSpatialStructure()
{
    staticModified = false;

    delete sceneOctree;

    CreateStaticSpatialDataStruct();
}

void Scene::UpdateDynamicSpatialStructure()
{
    dynamicModified = false;

    delete dynamicTree;

    CreateDynamicSpatialDataStruct();
}

void Scene::CheckObjectsInFrustum(std::vector<GameObject*>& outRenderGameObjects, FrustumPlanes frustumPlanes) const
{
#ifdef OPTICK
    OPTICK_CATEGORY("Scene::CheckObjectsInFrustum", Optick::Category::GameLogic)
#endif
    std::vector<GameObject*> queriedObjects;

    sceneOctree->QueryElements<FrustumPlanes>(frustumPlanes, queriedObjects);

    dynamicTree->QueryElements<FrustumPlanes>(frustumPlanes, queriedObjects);

    for (auto gameObject : queriedObjects)
    {
        OBB objectOBB = gameObject->GetGlobalOBB();

        if (frustumPlanes.Intersects(objectOBB))
        {
            outRenderGameObjects.push_back(gameObject);
        }
    }
}

GameObject* Scene::GetGameObjectByUID(UID gameObjectUUID)
{
    if (gameObjectsContainer.count(gameObjectUUID))
    {
        return gameObjectsContainer[gameObjectUUID];
    }
    return nullptr;
}

GameObject* Scene::GetGameObjectByName(const std::string& name)
{
    // TODO: Replace gameObject name to a HashString, I've seen it is also compared in some scripts and would improve
    // performance

    // Returns the first object with that name, if there are more they are ignored
    for (const auto& obj : gameObjectsContainer)
    {
        if (obj.second->GetName() == name) return obj.second;
    }

    // GLOG("[WARNING] No gameObject found with name %s", name.c_str());
    return nullptr;
}

//Loops in the Parent Tree node and try to find targetName GO
GameObject* Scene::GetGameObjectByParentNameAndTargetName(const std::string& parentName, const std::string& targetName)
{
    // TODO: Replace gameObject name to a HashString, I've seen it is also compared in some scripts and would improve
    // performance

    GameObject* parentGO = GetGameObjectByName(parentName);
    if (!parentGO) return nullptr;

    std::vector<UID> stack;
    const auto& childrenVectorUID = parentGO->GetChildren();
    stack.insert(stack.end(), childrenVectorUID.begin(), childrenVectorUID.end());

    std::unordered_set<UID> visited;
    visited.reserve(stack.size() * 2 + 16);

    while (!stack.empty())
    {
        UID uid = stack.back();
        stack.pop_back();

        if (!visited.insert(uid).second) continue;

        GameObject* cGO = GetGameObjectByUID(uid);
        if (!cGO) continue;

        if (cGO->GetName() == targetName) return cGO;

        const auto& kids = cGO->GetChildren();
        for (auto it = kids.rbegin(); it != kids.rend(); ++it)
        {
            if (!visited.count(*it)) stack.push_back(*it);
        }
    }

    // GLOG("[WARNING] No gameObject found with name %s", name.c_str());
    return nullptr;
}

CameraComponent* Scene::GetMainCamera() const
{
    if (mainCamera != nullptr)
    {
        if (mainCamera->IsEffectivelyEnabled()) return mainCamera;
    }
    return nullptr;
}

CameraComponent* Scene::GetMainCameraEvenDisabled() const
{
    if (mainCamera != nullptr) return mainCamera;

    return nullptr;
}

void Scene::LoadModel(const UID modelUID)
{
    if (modelUID != INVALID_UID)
    {
        // GLOG("Load model %llu", modelUID);

        ResourceModel* newModel               = (ResourceModel*)App->GetResourcesModule()->RequestResource(modelUID);
        const Model& model                    = newModel->GetModelData();
        const std::vector<int>& rootNodesIdx  = model.GetRootNodesIdx();
        const std::vector<NodeData>& allNodes = model.GetNodes();

        std::vector<GameObject*> gameObjectsArray;
        gameObjectsArray.resize(allNodes.size());
        std::vector<UID> gameObjectsUID;
        std::vector<GameObject*> rootGameObjects;

        // GLOG("Model Animation UID: %llu", newModel->GetAnimationUID());

        const auto& animUIDs = newModel->GetAllAnimationUIDs();
        // GLOG("Total Animation UIDs %zu ", animUIDs.size());

        /*
        for (UID uid : animUIDs)
        {
            GLOG("Animation UID in list: %llu ", uid);
        }
        */
        for (unsigned int i = 0; i < allNodes.size(); ++i)
        {
            gameObjectsUID.push_back(GenerateUID());
        }

        for (int rootNodeIdx : rootNodesIdx)
        {
            const NodeData& rootNode = allNodes[rootNodeIdx];

            std::vector<NodeParent> nodesToVisit;
            nodesToVisit.push_back({rootNodeIdx, rootNode.parentIndex});

            while (!nodesToVisit.empty())
            {
                NodeParent currentNode = nodesToVisit.back();
                nodesToVisit.pop_back();

                const int currentNodeIndex      = currentNode.nodeID;
                const int currentParentIndex    = currentNode.parentID;

                const NodeData& currentNodeData = allNodes[currentNodeIndex];

                if (currentParentIndex == -1)
                {
                    GameObject* rootObject =
                        new GameObject(GetGameObjectRootUID(), currentNodeData.name, gameObjectsUID[currentNodeIndex]);
                    rootObject->SetLocalTransform(currentNodeData.transform);
                    // Add the gameObject to the rootObject
                    GetGameObjectByUID(GetGameObjectRootUID())->AddGameObject(rootObject->GetUID());
                    AddGameObject(rootObject->GetUID(), rootObject);
                    rootGameObjects.push_back(rootObject);
                    gameObjectsArray[currentNodeIndex] = rootObject;
                }
                else
                {
                    GameObject* gameObject = new GameObject(
                        gameObjectsUID[currentParentIndex], currentNodeData.name, gameObjectsUID[currentNodeIndex]
                    );
                    gameObject->SetLocalTransform(currentNodeData.transform);
                    GetGameObjectByUID(gameObject->GetParent())->AddGameObject(gameObject->GetUID());
                    AddGameObject(gameObject->GetUID(), gameObject);

                    gameObjectsArray[currentNodeIndex] = gameObject;
                }

                for (auto it = currentNodeData.children.rbegin(); it != currentNodeData.children.rend(); ++it)
                {
                    nodesToVisit.push_back({*it, currentNodeIndex});
                }
            }
        }

        // Iterate again to add the meshes and skins.
        // Can't be done in the same loop because the bones have to be already created
        for (int rootNodeIdx : rootNodesIdx)
        {
            const NodeData& rootNode = allNodes[rootNodeIdx];

            std::vector<NodeParent> nodesToVisit;
            nodesToVisit.push_back({rootNodeIdx, rootNode.parentIndex});

            while (!nodesToVisit.empty())
            {
                NodeParent currentNode = nodesToVisit.back();
                nodesToVisit.pop_back();

                const int currentNodeIndex      = currentNode.nodeID;
                const int currentParentIndex    = currentNode.parentID;

                const NodeData& currentNodeData = allNodes[currentNodeIndex];

                if (currentNodeData.meshes.size() > 0)
                {
                    GameObject* currentGameObject = gameObjectsArray[currentNodeIndex];
                    // GLOG("Node %s has %d meshes", currentNodeData.name.c_str(), currentNodeData.meshes.size());

                    unsigned meshNum              = 1;

                    for (const auto& mesh : currentNodeData.meshes)
                    {
                        GameObject* meshObject = nullptr;
                        if (currentNodeData.meshes.size() > 1)
                        {
                            meshObject = new GameObject(
                                currentGameObject->GetUID(),
                                currentGameObject->GetName() + " Mesh " + std::to_string(meshNum)
                            );
                            ++meshNum;

                            gameObjectsArray.push_back(meshObject);
                        }
                        else
                        {
                            meshObject = currentGameObject;
                        }

                        if (meshObject->CreateComponent(COMPONENT_MESH))
                        {
                            if (currentNodeData.meshes.size() > 1)
                            {
                                currentGameObject->AddGameObject(meshObject->GetUID());
                                AddGameObject(meshObject->GetUID(), meshObject);
                            }

                            MeshComponent* meshComponent = meshObject->GetComponent<MeshComponent*>();
                            meshComponent->SetModelUID(modelUID);
                            meshComponent->AddMesh(mesh.first, false);
                            meshComponent->AddMaterial(mesh.second);

                            // Add skin to meshComponent
                            if (currentNodeData.skinIndex != -1)
                            {
                                /*GLOG(
                                    "Node %s has skin index: %d", currentNodeData.name.c_str(),
                                    currentNodeData.skinIndex
                                );*/
                                Skin skin = model.GetSkin(currentNodeData.skinIndex);

                                std::vector<GameObject*> bones;
                                std::vector<UID> bonesIds;
                                for (int index : skin.bonesIndices)
                                {
                                    bonesIds.push_back(gameObjectsArray[index]->GetUID());
                                    bones.push_back(gameObjectsArray[index]);
                                }
                                meshComponent->SetBones(bones, bonesIds);
                                meshComponent->SetBindMatrices(skin.inverseBindMatrices);
                                meshComponent->SetSkinIndex(currentNodeData.skinIndex);
                            }
                        }
                    }
                }

                for (auto it = currentNodeData.children.rbegin(); it != currentNodeData.children.rend(); ++it)
                {
                    nodesToVisit.push_back({*it, currentNodeIndex});
                }
            }
        }
        for (GameObject* rootGameObject : rootGameObjects)
        {
            if (!animUIDs.empty())
            {
                rootGameObject->CreateComponent(COMPONENT_ANIMATION);
                AnimationComponent* animComponent = rootGameObject->GetComponent<AnimationComponent*>();

                // GLOG("Model has %zu animations", animUIDs.size());
                for (UID uid : animUIDs)
                {
                    // GLOG("Setting aimation resource with UID %llu ", uid);
                    animComponent->SetAnimationResource(uid);

                    // GLOG("Animation UID: %llu", uid);
                }
            }
            else
            {
                GLOG("No animations found for this model");
            }
            rootGameObject->UpdateTransformForGOBranch();
        }

        std::set<UID> visitedUID;

        // SET CHILD GAME OBJECTS TO SELECT THE PARENT
        for (int i = 0; i < gameObjectsArray.size(); ++i)
        {
            if (visitedUID.find(gameObjectsArray[i]->GetUID()) == visitedUID.end())
            {
                visitedUID.insert(gameObjectsArray[i]->GetUID());

                std::stack<UID> childrenToVisit;

                // ADDING CHILDREN TO START ITERATION FOR PARENT CHECKBOX SELECTION
                for (const UID& currentChild : gameObjectsArray[i]->GetChildren())
                {
                    childrenToVisit.push(currentChild);
                }

                while (!childrenToVisit.empty())
                {
                    const UID currentUID = childrenToVisit.top();
                    childrenToVisit.pop();
                    visitedUID.insert(currentUID);

                    GameObject* currentGameObject = GetGameObjectByUID(currentUID);

                    currentGameObject->SetSelectParent(true);

                    // ADDING CHILDREN TO START ITERATION FOR PARENT CHECKBOX SELECTION
                    for (const UID& currentChild : currentGameObject->GetChildren())
                    {
                        if (visitedUID.find(currentChild) == visitedUID.end()) childrenToVisit.push(currentChild);
                    }
                }
            }
        }
    }
}

void Scene::LoadNestedPrefab(GameObject* prefabRoot)
{
    std::vector<UID> parentsToUpdate; // Contains all uids of gos which are a prefab root
    parentsToUpdate.push_back(prefabRoot->GetUID());
    std::stack<GameObject*> prefabBuffer;   // Prefab uids for iterative iteration
    prefabBuffer.push(prefabRoot);

    while (!prefabBuffer.empty())
    {
        GameObject* replacementTarget = prefabBuffer.top();
        const ResourcePrefab* resourcePrefab      = dynamic_cast<ResourcePrefab*>(
            App->GetResourcesModule()->RequestResource(prefabBuffer.top()->GetPrefabUID()));
        if (resourcePrefab == nullptr)
        {
            GLOG("Prefab for UID %d not found", prefabBuffer.top())
            continue;
        }

        prefabBuffer.pop();
        const std::unordered_map<UID, GameObject*>& prefabGOContainer = resourcePrefab->GetGameObjectsContainer();
        // All uids of the prefab will be summed with this to generate new unique ids for the scene game object tree
        const UID staticModUID = GenerateUID();

        std::stack<UID> prefabChildBuffer;

        GameObject* replacement = new GameObject(replacementTarget->GetParent(), resourcePrefab->GetRootObject(), false);
        replacement->ModifyAllUIDsBy(staticModUID);
        replacement->SetUID(replacementTarget->GetUID());
        replacement->SetParent(replacementTarget->GetParent());
        replacement->SetLocalTransform(replacementTarget->GetLocalTransform(), false);
        gameObjectsContainer.insert({replacement->GetUID(), replacement});

        for (UID child: replacement->GetChildren())
            prefabChildBuffer.push(child);
        
        while (!prefabChildBuffer.empty())
        {
            GameObject* childGO = prefabGOContainer.at(prefabChildBuffer.top());
            prefabChildBuffer.pop();
            GameObject* clonedGameObject = new GameObject(childGO->GetParent(), childGO, false);
            clonedGameObject->ModifyAllUIDsBy(staticModUID);
            if (clonedGameObject->GetPrefabUID() != INVALID_UID)
            {
                parentsToUpdate.push_back(clonedGameObject->GetUID());
                prefabBuffer.push(clonedGameObject);
            } else 
                gameObjectsContainer.insert({clonedGameObject->GetUID(), clonedGameObject});
        }
    }

    for (UID parentToUpdate: parentsToUpdate)
    {
        for (UID rootChild: gameObjectsContainer[parentToUpdate]->GetChildren())
            gameObjectsContainer[rootChild]->SetParent(parentToUpdate);
    }
    gameObjectsContainer[prefabRoot->GetUID()]->SetEnabledRecursive(prefabRoot->IsEnabled());
}

void Scene::AddPrefab(UID prefabUID)
{
    GameObject* parent = GetSelectedGameObject();
    if (parent == nullptr) parent = GetGameObjectByUID(gameObjectRootUID);
    GameObject* prefabBaseGO = new GameObject(parent->GetUID(), "Base");
    prefabBaseGO->SetPrefabUID(prefabUID);
    gameObjectsContainer.insert({prefabBaseGO->GetUID(), prefabBaseGO});
    parent->AddGameObject(prefabBaseGO->GetUID());

    LoadNestedPrefab(prefabBaseGO);

    prefabBaseGO->UpdateTransformForGOBranch();
}

void Scene::UpdatePrefab(UID prefabUID)
{
    App->GetResourcesModule()->ForceUnloadResource(prefabUID);
    
    for (const auto& gameObject : gameObjectsContainer)
    {
        if (gameObject.second != nullptr && gameObject.second->GetPrefabUID() == prefabUID)
        {
            for (UID child: gameObject.second->GetChildren())
                RemoveGameObjectHierarchy(child);
            LoadNestedPrefab(gameObject.second);
        }
    }
    
    if (lightsConfig != nullptr) lightsConfig->GetAllSceneLights();
}

void Scene::DeletePrefab(const UID prefabUID) const
{
    for (const auto& gameObject : gameObjectsContainer)
    {
        if (gameObject.second != nullptr && gameObject.second->GetPrefabUID() == prefabUID)
        {
            gameObject.second->RemovePrefabStatus();
            App->GetResourcesModule()->ReleaseResource(prefabUID);
        }
    }

    App->GetLibraryModule()->DeletePrefabFiles(prefabUID);
}

void Scene::QueueGameObjectDelete(UID uid)
{
    if (uid != INVALID_UID) pendingDeletes.push_back(uid);
}
void Scene::FlushPendingDeletes()
{
    for (UID id : pendingDeletes)
        RemoveGameObjectHierarchy(id);
    pendingDeletes.clear();
}

template <typename T> std::vector<T> Scene::GetEnabledComponentsOfType() const
{
    std::vector<T> result;

    for (const auto& [uid, go] : gameObjectsContainer)
    {
        if (!go || !go->IsGloballyEnabled()) continue;

        T comp = go->GetComponent<T>();
        if (comp && comp->GetEnabled())
        {
            result.push_back(comp);
        }
    }

    return result;
}

template std::vector<DirectionalLightComponent*> Scene::GetEnabledComponentsOfType<DirectionalLightComponent*>() const;
template std::vector<PointLightComponent*> Scene::GetEnabledComponentsOfType<PointLightComponent*>() const;
template std::vector<SpotLightComponent*> Scene::GetEnabledComponentsOfType<SpotLightComponent*>() const;
