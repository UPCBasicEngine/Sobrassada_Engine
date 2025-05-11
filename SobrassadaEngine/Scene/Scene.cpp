#include "Scene.h"

#include "Application.h"
#include "BatchManager.h"
#include "CameraComponent.h"
#include "CameraModule.h"
#include "Component.h"
#include "ComponentUtils.h"
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
#include "PathfinderModule.h"
#include "PhysicsModule.h"
#include "ProjectModule.h"
#include "Quadtree.h"
#include "Resource.h"
#include "ResourceModel.h"
#include "ResourcePrefab.h"
#include "ResourcesModule.h"
#include "SceneModule.h"
#include "ScriptComponent.h"
#include "ShaderModule.h"
#include "Standalone/AnimationComponent.h"
#include "Standalone/Lights/DirectionalLightComponent.h"
#include "Standalone/Lights/PointLightComponent.h"
#include "Standalone/Lights/SpotLightComponent.h"
#include "Standalone/MeshComponent.h"

#include "SDL_mouse.h"
#include "glew.h"
#include "imgui.h"
#include "imgui_internal.h"
// guizmo after imgui include
#include "ImGuizmo.h"
#ifdef OPTICK
#include "optick.h"
#endif

#include <set>

Scene::Scene(const char* sceneName) : sceneUID(GenerateUID())
{
    this->sceneName             = sceneName;

    GameObject* sceneGameObject = new GameObject("SceneModule GameObject");
    selectedGameObjectUID = gameObjectRootUID = sceneGameObject->GetUID();

    gameObjectsContainer.insert({sceneGameObject->GetUID(), sceneGameObject});

    lightsConfig = new LightsConfig();
}

Scene::Scene(const rapidjson::Value& initialState, UID loadedSceneUID) : sceneUID(loadedSceneUID)
{
    this->sceneName       = initialState["Name"].GetString();
    gameObjectRootUID     = initialState["RootGameObject"].GetUint64();
    selectedGameObjectUID = gameObjectRootUID;
    if (initialState.HasMember("NavmeshUID")) navmeshUID = initialState["NavmeshUID"].GetUint64();

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
            gameObjectsContainer.insert({newGameObject->GetUID(), newGameObject});

            gameObjectDataMap[newGameObject->GetUID()] = &gameObject;
        }
    }

    // Deserialize Lights Config
    if (initialState.HasMember("Lights Config") && initialState["Lights Config"].IsObject())
    {
        lightsConfig = new LightsConfig();
        lightsConfig->LoadData(initialState["Lights Config"]);
    }

    GLOG("%s scene loaded", sceneName.c_str());
}

Scene::~Scene()
{
    App->GetPhysicsModule()->EmptyWorld();

    for (auto it = gameObjectsContainer.begin(); it != gameObjectsContainer.end(); ++it)
    {
        delete it->second;
    }
    gameObjectsContainer.clear();

    selectedGameObjects.clear();

    delete lightsConfig;
    delete sceneOctree;
    delete dynamicTree;

    lightsConfig = nullptr;
    sceneOctree  = nullptr;
    dynamicTree  = nullptr;

    GLOG("%s scene closed", sceneName.c_str());
}

void Scene::Init()
{
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

    for (const UID prefab : prefabs)
    {
        OverridePrefabs(prefab);
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

    lightsConfig->InitSkybox();
    lightsConfig->InitLightBuffers();

    // Call this after overriding the prefabs to avoid duplicates in gameObjectsToUpdate
    GetGameObjectByUID(gameObjectRootUID)->UpdateTransformForGOBranch();

    UpdateStaticSpatialStructure();
    UpdateDynamicSpatialStructure();

    multiSelectParent = new GameObject(GenerateUID(), "MULTISELECT_DUMMY");
    gameObjectsContainer.insert({multiSelectParent->GetUID(), multiSelectParent});
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

    App->GetPhysicsModule()->SaveLayerData(targetState, allocator);

    // Serialize GameObjects
    rapidjson::Value gameObjectsJSON(rapidjson::kArrayType);

    for (auto it = gameObjectsContainer.begin(); it != gameObjectsContainer.end(); ++it)
    {
        if (it->second != nullptr && it->second != multiSelectParent)
        {
            rapidjson::Value goJSON(rapidjson::kObjectType);

            it->second->Save(goJSON, allocator);

            gameObjectsJSON.PushBack(goJSON, allocator);
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
        }
        App->GetSceneModule()->ResetOnlyOnceInPlayMode();
    }

    for (auto& gameObject : gameObjectsContainer)
        gameObject.second->UpdateComponents(deltaTime);

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
        if (mainCamera->GetEnabled() && mainCamera->IsEffectivelyEnabled()) RenderScene(deltaTime, mainCamera);
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
    Framebuffer* framebuffer = App->GetSceneModule()->GetInPlayMode() ? App->GetOpenGLModule()->GetFramebuffer()
                             : camera != nullptr                      ? camera->GetFramebuffer()
                                                                      : App->GetOpenGLModule()->GetFramebuffer();

    std::vector<GameObject*> objectsToRender;
    CheckObjectsToRender(objectsToRender, camera);

#ifdef OPTICK
    OPTICK_CATEGORY("Scene::MeshesToRender", Optick::Category::GameLogic)
#endif
    glEnable(GL_STENCIL_TEST);

    if (App->GetDebugDrawModule()->GetDebugOptionValue(static_cast<int>(DebugOptions::RENDER_WIREFRAME)))
    {
        App->GetOpenGLModule()->SetRenderWireframe(true);
        GeometryPassRender(objectsToRender, camera, gbuffer);
        App->GetOpenGLModule()->SetRenderWireframe(false);
    }
    else GeometryPassRender(objectsToRender, camera, gbuffer);

    LightingPassRender(objectsToRender, camera, gbuffer, framebuffer);

    {
#ifdef OPTICK
        OPTICK_CATEGORY("Scene::GameObject::Render", Optick::Category::Rendering)
#endif
        for (const auto& gameObject : objectsToRender)
        {
            if (gameObject != nullptr)
            {
                gameObject->Render(deltaTime);
            }
        }
    }

    {
#ifdef OPTICK
        OPTICK_CATEGORY("Scene::GameObject::DrawGizmos", Optick::Category::Rendering)
#endif
        for (const auto& gameObject : gameObjectsContainer)
        {
            gameObject.second->DrawGizmos();
        }
    }

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
        gameTimer->TogglePause();
    }
    ImGui::SameLine();
    if (ImGui::Button("Step"))
    {
        stepPlaying = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Stop"))
    {
        stopPlaying = true;
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

void Scene::AddGameObjectToUpdate(GameObject* gameObject)
{
    if (!gameObject->WillUpdate())
    {
        gameObject->SetWillUpdate(true);
        gameObjectsToUpdate.push_back(gameObject);
    }
}

void Scene::UpdateGameObjects()
{
    for (GameObject* gameObject : gameObjectsToUpdate)
    {
        if (gameObject)
        {
            gameObject->ParentUpdatedComponents();
            gameObject->SetWillUpdate(false);
        }
    }
    gameObjectsToUpdate.clear();
}

void Scene::ClearGameObjectsToUpdate()
{
    gameObjectsToUpdate.clear();
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
    ClearGameObjectsToUpdate();
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
    int nodeCapacity = 5;
    dynamicTree      = new Quadtree(center, length, nodeCapacity);

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

void Scene::CheckObjectsToRender(std::vector<GameObject*>& outRenderGameObjects, CameraComponent* camera) const
{
#ifdef OPTICK
    OPTICK_CATEGORY("Scene::CheckObjectsToRender", Optick::Category::GameLogic)
#endif
    std::vector<GameObject*> queriedObjects;

    FrustumPlanes frustumPlanes;
    if (camera == nullptr) frustumPlanes = App->GetCameraModule()->GetFrustrumPlanes();
    else frustumPlanes = camera->GetFrustrumPlanes();

    sceneOctree->QueryElements<FrustumPlanes>(frustumPlanes, queriedObjects);

    dynamicTree->QueryElements<FrustumPlanes>(frustumPlanes, queriedObjects);

    for (auto gameObject : queriedObjects)
    {
        OBB objectOBB = gameObject->GetGlobalOBB();

        if (frustumPlanes.Intersects(objectOBB)) outRenderGameObjects.push_back(gameObject);
    }
}

void Scene::GeometryPassRender(
    const std::vector<GameObject*>& objectsToRender, CameraComponent* camera, GBuffer* gbuffer
) const
{
    gbuffer->Bind();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilMask(0xFF);

    glDisable(GL_BLEND);

    BatchManager* batchManager = App->GetResourcesModule()->GetBatchManager();
    std::vector<MeshComponent*> meshesToRender;

    for (const auto& gameObject : objectsToRender)
    {
        MeshComponent* mesh = gameObject->GetComponent<MeshComponent*>();
        if (mesh != nullptr && mesh->GetEnabled() && mesh->GetBatch() != nullptr) meshesToRender.push_back(mesh);
    }

    batchManager->Render(meshesToRender, camera);
    gbuffer->Unbind();

    glEnable(GL_BLEND);
}

void Scene::LightingPassRender(
    const std::vector<GameObject*>& renderGameObjects, CameraComponent* camera, GBuffer* gbuffer,
    Framebuffer* framebuffer
) const
{
    // LIGHTING PASS
#ifndef GAME
    framebuffer->Bind();
#else
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif

    // SKYBOX
    if (!App->GetDebugDrawModule()->GetDebugOptionValue((int)DebugOptions::RENDER_WIREFRAME))
    {
        float4x4 projection;
        float4x4 view;

        if (camera == nullptr)
            lightsConfig->RenderSkybox(
                App->GetCameraModule()->GetProjectionMatrix(), App->GetCameraModule()->GetViewMatrix()
            );
        else
        {
            bool change = false;
            // Cubemap does not support Ortographic projection
            if (camera->GetFrustumType() == 1)
            {
                change = true;
                camera->ChangeToPerspective();
            }
            lightsConfig->RenderSkybox(camera->GetProjectionMatrix(), camera->GetViewMatrix());
            if (change) camera->ChangeToOrtographic();
        }
    }

    // COPYING DEPTH BUFFER AND STENCIL FROM GBUFFER TO RENDER FRAMEBUFFER
    // TODO CHECK IF GAME RELEASE TO RENDER TO DEFAULT BUFFER INSTEAD OF FRAMEBUFFER
    unsigned int width  = framebuffer->GetTextureWidth();
    unsigned int height = framebuffer->GetTextureHeight();

    glBindFramebuffer(GL_READ_FRAMEBUFFER, gbuffer->gBufferObject);

#ifndef GAME
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer->GetFramebufferID()); // write to default framebuffer
#else
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
#endif

    glBlitFramebuffer(
        0, 0, width, height, 0, 0, width, height, GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, GL_NEAREST
    );

#ifndef GAME
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer->GetFramebufferID()); // write to default framebuffer
#else
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif

    // SETTING STENCIL TEST FOR ONLY RENDER TO GBUFFER FRAGMENTS WRITES
    glStencilFunc(GL_EQUAL, 1, 0xFF);
    glStencilMask(0xFF);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gbuffer->diffuseTexture);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gbuffer->specularTexture);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, gbuffer->positionTexture);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, gbuffer->normalTexture);

    lightsConfig->SetLightsShaderData();

    unsigned int lightingPassProgram = App->GetShaderModule()->GetLightingPassProgram();

    glUseProgram(lightingPassProgram);

    float3 cameraPos;
    if (camera == nullptr) cameraPos = App->GetCameraModule()->GetCameraPosition();
    else cameraPos = camera->GetCameraPosition();

    glUniform3fv(glGetUniformLocation(lightingPassProgram, "cameraPos"), 1, &cameraPos[0]);

    App->GetOpenGLModule()->DrawArrays(GL_TRIANGLES, 0, 3);

    glDisable(GL_STENCIL_TEST);

    // COPYING DEPTH BUFFER FROM GBUFFER TO RENDER FRAMEBUFFER
    glBindFramebuffer(GL_READ_FRAMEBUFFER, gbuffer->gBufferObject);

#ifndef GAME
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer->GetFramebufferID()); // write to default framebuffer
#else
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
#endif
    glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_DEPTH_BUFFER_BIT, GL_NEAREST);

#ifndef GAME
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer->GetFramebufferID()); // write to default framebuffer
#else
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif
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

    GLOG("[WARNING] No gameObject found with name %s", name.c_str());
    return nullptr;
}

void Scene::LoadModel(const UID modelUID)
{
    if (modelUID != INVALID_UID)
    {
        GLOG("Load model %llu", modelUID);

        ResourceModel* newModel               = (ResourceModel*)App->GetResourcesModule()->RequestResource(modelUID);
        const Model& model                    = newModel->GetModelData();
        const std::vector<int>& rootNodesIdx  = model.GetRootNodesIdx();
        const std::vector<NodeData>& allNodes = model.GetNodes();

        std::vector<GameObject*> gameObjectsArray;
        gameObjectsArray.resize(allNodes.size());
        std::vector<UID> gameObjectsUID;
        std::vector<GameObject*> rootGameObjects;

        GLOG("Model Animation UID: %llu", newModel->GetAnimationUID());

        const auto& animUIDs = newModel->GetAllAnimationUIDs();
        GLOG("Total Animation UIDs %zu ", animUIDs.size());

        for (UID uid : animUIDs)
        {
            GLOG("Animation UID in list: %llu ", uid);
        }
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
                    GLOG("Node %s has %d meshes", currentNodeData.name.c_str(), currentNodeData.meshes.size());

                    unsigned meshNum = 1;

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
                                GLOG(
                                    "Node %s has skin index: %d", currentNodeData.name.c_str(),
                                    currentNodeData.skinIndex
                                );
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

                GLOG("Model has %zu animations", animUIDs.size());
                for (UID uid : animUIDs)
                {
                    GLOG("Setting aimation resource with UID %llu ", uid);
                    animComponent->SetAnimationResource(uid);

                    GLOG("Animation UID: %llu", uid);
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

void Scene::LoadPrefab(const UID prefabUID, const ResourcePrefab* prefab, const float4x4& transform)
{
    if (prefabUID != INVALID_UID)
    {
        std::map<UID, UID> remappingTable; // Reference UID | New GameObject UID

        const ResourcePrefab* resourcePrefab =
            prefab == nullptr ? (const ResourcePrefab*)App->GetResourcesModule()->RequestResource(prefabUID) : prefab;
        const std::vector<GameObject*>& referenceObjects = resourcePrefab->GetGameObjectsVector();
        const std::vector<int>& parentIndices            = resourcePrefab->GetParentIndices();

        std::vector<GameObject*> newObjects;
        newObjects.push_back(new GameObject(GetGameObjectRootUID(), referenceObjects[0]));

        // If new, always appear at origin. If overriden, stay in place
        if (prefab != nullptr)
        {
            newObjects[0]->SetLocalTransform(transform);
        }
        else
        {
            // This probably won't be needed when gltfDefaults are there, but for now it is
            float4x4 newTrans = newObjects[0]->GetLocalTransform();
            newTrans.SetTranslatePart(float3(0, 0, 0));
            newObjects[0]->SetLocalTransform(newTrans);
        }

        // First instantiate all gameObjects and components
        newObjects[0]->SetPrefabUID(prefabUID);
        GetGameObjectByUID(GetGameObjectRootUID())->AddGameObject(newObjects[0]->GetUID());
        AddGameObject(newObjects[0]->GetUID(), newObjects[0]);
        remappingTable.insert({referenceObjects[0]->GetUID(), newObjects[0]->GetUID()});

        for (int i = 1; i < referenceObjects.size(); ++i)
        {
            UID parentUID = newObjects[parentIndices[i]]->GetUID();
            newObjects.push_back(new GameObject(parentUID, referenceObjects[i]));
            newObjects[parentIndices[i]]->AddGameObject(newObjects[i]->GetUID());
            AddGameObject(newObjects[i]->GetUID(), newObjects[i]);
            remappingTable.insert({referenceObjects[i]->GetUID(), newObjects[i]->GetUID()});
            newObjects[i]->SetEnabled(referenceObjects[i]->IsEnabled());
        }

        // Then do a second loop to update all components UIDs reference (ex. skinning)
        for (int i = 0; i < newObjects.size(); ++i)
        {
            MeshComponent* mesh = referenceObjects[i]->GetComponent<MeshComponent*>();
            if (mesh != nullptr && mesh->GetBones().size() > 0)
            {
                // Remap the bones references
                const std::vector<UID>& bones = mesh->GetBones();
                std::vector<UID> newBonesUIDs;
                std::vector<GameObject*> newBonesObjects;

                for (const UID bone : bones)
                {
                    const UID uid = remappingTable.find(bone)->second;
                    newBonesUIDs.push_back(uid);
                    newBonesObjects.push_back(GetGameObjectByUID(uid));
                }

                // This should never be nullptr
                MeshComponent* newMesh = newObjects[i]->GetComponent<MeshComponent*>();
                newMesh->SetBones(newBonesObjects, newBonesUIDs);
            }

            // If has animations, map them here
            AnimationComponent* animComp = newObjects[i]->GetComponent<AnimationComponent*>();
            if (animComp) animComp->SetBoneMapping();
        }

        if (prefab == nullptr) App->GetResourcesModule()->ReleaseResource(resourcePrefab);
        newObjects[0]->UpdateTransformForGOBranch();

        // Get all scene lights, because if the prefab has lights when creating them they won't be added to the
        // scene, as the gameObject is still not part of the scene
        if (lightsConfig != nullptr) lightsConfig->GetAllSceneLights();
    }
}

void Scene::OverridePrefabs(const UID prefabUID)
{
    const ResourcePrefab* prefab = (const ResourcePrefab*)App->GetResourcesModule()->RequestResource(prefabUID);

    // If prefab is null, it no longer exists, then remove the prefab UID from all objects that may have it
    if (prefab == nullptr)
    {
        for (const auto& gameObject : gameObjectsContainer)
        {
            if (gameObject.second != nullptr && gameObject.second->GetPrefabUID() == prefabUID)
                gameObject.second->SetPrefabUID(INVALID_UID);
        }
        return;
    }

    // Store uids and transforms. We need transforms so when we override the prefab, the objects
    // stay in place. UIDs to delete the duplicates
    std::vector<UID> updatedObjects;
    std::vector<float4x4> transforms;
    for (const auto& gameObject : gameObjectsContainer)
    {
        if (gameObject.second != nullptr)
        {
            if (gameObject.second->GetPrefabUID() == prefabUID)
            {
                updatedObjects.push_back(gameObject.first);
                transforms.emplace_back(gameObject.second->GetLocalTransform());
            }
        }
    }

    for (const UID object : updatedObjects)
    {
        RemoveGameObjectHierarchy(object);
    }

    for (const float4x4& transform : transforms)
    {
        LoadPrefab(prefabUID, prefab, transform);
    }

    App->GetResourcesModule()->ReleaseResource(prefab);
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
