#include "SceneModule.h"

#include "Application.h"
#include "CameraModule.h"
#include "Config/EngineConfig.h"
#include "Config/ProjectConfig.h"
#include "EditorUIModule.h"
#include "FileSystem.h"
#include "GameObject.h"
#include "GameTimer.h"
#include "ImGuizmo.h"
#include "InputModule.h"
#include "LibraryModule.h"
#include "PathfinderModule.h"
#include "PhysicsModule.h"
#include "ProjectModule.h"
#include "RaycastController.h"
#include "ResourcesModule.h"
#include "Standalone/AnimationComponent.h"
#include "Standalone/Lights/DirectionalLightComponent.h"
#include "Standalone/Lights/PointLightComponent.h"
#include "Standalone/Lights/SpotLightComponent.h"
#include "Standalone/MeshComponent.h"

#include <SDL_mouse.h>
#include <map>
#include <queue>
#include <tuple>
#ifdef OPTICK
#include "optick.h"
#endif

SceneModule::SceneModule()
{
}

SceneModule::~SceneModule()
{
}

bool SceneModule::Init()
{
    if (!App->GetProjectModule()->IsProjectLoaded()) return true;

    const std::string& startupScene = App->GetProjectModule()->GetProjectConfig()->GetStartupScene();
    bool startupSceneLoaded =
        !startupScene.empty() && App->GetLibraryModule()->LoadScene((startupScene + SCENE_EXTENSION).c_str());

    if (!startupSceneLoaded) CreateScene();

    if (App->GetEngineConfig()->ShouldStartGameOnStartup()) loadedScene->SetStartPlaying(true);

    return true;
}

update_status SceneModule::PreUpdate(float deltaTime)
{
    return UPDATE_CONTINUE;
}

update_status SceneModule::Update(float deltaTime)
{
    if (loadedScene != nullptr)
    {
        return loadedScene->Update(deltaTime);
    }
    return UPDATE_CONTINUE;
}

update_status SceneModule::Render(float deltaTime)
{
    if (loadedScene != nullptr)
    {
#ifdef OPTICK
        OPTICK_CATEGORY("SceneModule::Render", Optick::Category::Rendering)
#endif
        return loadedScene->Render(deltaTime);
    }

    return UPDATE_CONTINUE;
}

update_status SceneModule::RenderEditor(float deltaTime)
{
#ifndef GAME
    if (loadedScene != nullptr)
    {
        return loadedScene->RenderEditor(deltaTime);
    }
#endif
    return UPDATE_CONTINUE;
}

update_status SceneModule::PostUpdate(float deltaTime)
{
    if (App->GetProjectModule()->IsProjectLoaded())
    {

        const KeyState* mouseButtons = App->GetInputModule()->GetMouseButtons();
        const KeyState* keyboard     = App->GetInputModule()->GetKeyboard();

        // CAST RAY WHEN LEFT CLICK IS RELEASED
        if (GetDoMouseInputsScene() && !ImGuizmo::IsUsingAny()) HandleRaycast(mouseButtons, keyboard);

        if (GetDoInputsGame())
        {
            if (mouseButtons[SDL_BUTTON_LEFT - 1] == KeyState::KEY_DOWN)
            {
                App->GetPathfinderModule()->HandleClickNavigation();
            }
        }

        // CTRL+D -> Duplicate selected game object
        if (keyboard[SDL_SCANCODE_LCTRL] && keyboard[SDL_SCANCODE_D] == KeyState::KEY_DOWN &&
            loadedScene->GetGameObjectRootUID() != loadedScene->GetSelectedGameObjectUID())
            HandleObjectDuplication();

        // Delete -> Delete selected game object
        if (keyboard[SDL_SCANCODE_DELETE] == KeyState::KEY_DOWN &&
            loadedScene->GetGameObjectRootUID() != loadedScene->GetSelectedGameObjectUID())
            HandleObjectDeletion();

        // CHECKING FOR UPDATED STATIC AND DYNAMIC OBJECTS
        GizmoDragState currentGizmoState = App->GetEditorUIModule()->GetImGuizmoDragState();
        if (currentGizmoState == GizmoDragState::RELEASED || currentGizmoState == GizmoDragState::IDLE)
            HandleTreesUpdates();

        // IF SCENE NOT FOCUSED AND WAS MULTISELECTING RELEASE
        if (loadedScene->IsMultiselecting() && !loadedScene->IsSceneFocused()) loadedScene->ClearObjectSelection();

        if (loadedScene->GetStopPlaying()) SwitchPlayMode(false);
        else if (loadedScene->GetStartPlaying()) SwitchPlayMode(true);
        else if (loadedScene->GetStepPlaying())
        {
            App->GetGameTimer()->Step();
            loadedScene->SetStepPlaying(false);
        }
    }

    return UPDATE_CONTINUE;
}

bool SceneModule::ShutDown()
{
    CloseScene();
    //GLOG("Destroying scene")
    return true;
}

void SceneModule::CreateScene()
{
    CloseScene();
    App->GetPathfinderModule()->ResetNavmesh();
    loadedScene = new Scene(DEFAULT_SCENE_NAME);
    loadedScene->Init();
}

void SceneModule::LoadScene(const rapidjson::Value& initialState, const bool forceReload)
{
    const UID extractedSceneUID = initialState["UID"].GetUint64();
    if (!forceReload && loadedScene != nullptr && loadedScene->GetSceneUID() == extractedSceneUID)
    {
        GLOG("Scene already loaded: %s", loadedScene->GetSceneName());
        return;
    }

    CloseScene();

    loadedScene = new Scene(initialState, extractedSceneUID);
    loadedScene->Init();
}

void SceneModule::CloseScene()
{
    if (inPlayMode)
    {
        std::string tmpScene = App->GetProjectModule()->GetLoadedProjectPath() + SCENES_PLAY_PATH +
                               std::to_string(loadedScene->GetSceneUID()) + SCENE_EXTENSION;
        FileSystem::Delete(tmpScene.c_str());
        inPlayMode = false;
    }

    // TODO Warning dialog before closing scene without saving
    delete loadedScene;
    loadedScene = nullptr;
    if (App->GetResourcesModule() != nullptr) App->GetResourcesModule()->ShutDown();
}

void SceneModule::SwitchPlayMode(bool play)
{
    if (play == inPlayMode || loadedScene == nullptr) return;

    if (inPlayMode)
    {
        std::string tmpScene = std::to_string(loadedScene->GetSceneUID()) + SCENE_EXTENSION;
        if (App->GetLibraryModule()->LoadScene(tmpScene.c_str(), true))
        {
            GLOG("----- Stopped Playing -----");
            FileSystem::Delete((App->GetProjectModule()->GetLoadedProjectPath() + SCENES_PLAY_PATH + tmpScene).c_str());
            App->GetGameTimer()->Reset();
            inPlayMode = false;
            loadedScene->SetStopPlaying(false);
        }
    }
    else
    {
        if (App->GetLibraryModule()->SaveScene("", SaveMode::SavePlayMode))
        {
            GLOG("----- Started Playing -----");
            App->GetGameTimer()->Start();
            inPlayMode       = true;
            onlyOncePlayMode = true;
            loadedScene->SetStartPlaying(false);
        }
    }
}

void SceneModule::HandleRaycast(const KeyState* mouseButtons, const KeyState* keyboard)
{
    if (mouseButtons[SDL_BUTTON_LEFT - 1] == KeyState::KEY_DOWN && !keyboard[SDL_SCANCODE_LALT] &&
        keyboard[SDL_SCANCODE_LSHIFT])
    {
        GameObject* selectedObject = RaycastController::GetRayIntersectionTrees<Octree, Quadtree>(
            App->GetCameraModule()->CastCameraRay(), loadedScene->GetOctree(), loadedScene->GetDynamicTree()
        );

        if (selectedObject != nullptr)
        {
            if (!loadedScene->IsMultiselecting()) loadedScene->SetMultiselectPosition(selectedObject->GetPosition());

            loadedScene->AddGameObjectToSelection(selectedObject->GetUID(), selectedObject->GetParent());
        }
    }
    else if (mouseButtons[SDL_BUTTON_LEFT - 1] == KeyState::KEY_DOWN && !keyboard[SDL_SCANCODE_LALT])
    {
        GameObject* selectedObject = RaycastController::GetRayIntersectionTrees<Octree, Quadtree>(
            App->GetCameraModule()->CastCameraRay(), loadedScene->GetOctree(), loadedScene->GetDynamicTree()
        );

        if (selectedObject != nullptr) loadedScene->SetSelectedGameObject(selectedObject->GetUID());

        loadedScene->ClearObjectSelection();
    }
}

void SceneModule::HandleObjectDuplication()
{
    std::vector<std::pair<UID, UID>> objectsToDuplicate; // GAME OBJECT | GAME OBJECT PARENT
    std::map<UID, UID> remappingTable;                   // Reference UID | New GameObject UID

    if (loadedScene->IsMultiselecting())
    {

        const std::map<UID, UID> selectedGameObjects = loadedScene->GetMultiselectedObjects();
        for (auto& childToDuplicate : selectedGameObjects)
        {
            objectsToDuplicate.push_back(childToDuplicate);
        }
    }
    else
    {
        objectsToDuplicate.push_back(
            {loadedScene->GetSelectedGameObject()->GetUID(), loadedScene->GetSelectedGameObject()->GetParent()}
        );
    }
    for (int indexToDuplicate = 0; indexToDuplicate < objectsToDuplicate.size(); ++indexToDuplicate)
    {
        std::vector<GameObject*> createdGameObjects;
        std::vector<GameObject*> originalGameObjects;

        GameObject* gameObjectToClone = loadedScene->GetGameObjectByUID(objectsToDuplicate[indexToDuplicate].first);
        GameObject* gameObjectToCloneParent =
            loadedScene->GetGameObjectByUID(objectsToDuplicate[indexToDuplicate].second);

        GameObject* clonedGameObject = new GameObject(objectsToDuplicate[indexToDuplicate].second, gameObjectToClone);

        remappingTable.insert({gameObjectToClone->GetUID(), clonedGameObject->GetUID()});
        createdGameObjects.push_back(clonedGameObject);
        originalGameObjects.push_back(gameObjectToClone);

        gameObjectToCloneParent->AddChildren(clonedGameObject->GetUID());
        loadedScene->AddGameObject(clonedGameObject->GetUID(), clonedGameObject);

        // CREATE DOWARDS HIERARCHY, FIRST ADD ALL CHILDREN (Parent, ChildrenUID)
        std::queue<std::pair<UID, UID>> gameObjectsToClone;

        for (UID child : gameObjectToClone->GetChildren())
        {
            gameObjectsToClone.push(std::make_pair(clonedGameObject->GetUID(), child));
        }

        Scene* scene = App->GetSceneModule()->GetScene();

        while (!gameObjectsToClone.empty())
        {
            std::pair<UID, UID> currentGameObjectPair = gameObjectsToClone.front();
            gameObjectsToClone.pop();

            gameObjectToClone       = loadedScene->GetGameObjectByUID(currentGameObjectPair.second);
            gameObjectToCloneParent = loadedScene->GetGameObjectByUID(currentGameObjectPair.first);

            clonedGameObject        = new GameObject(currentGameObjectPair.first, gameObjectToClone);

            remappingTable.insert({gameObjectToClone->GetUID(), clonedGameObject->GetUID()});
            createdGameObjects.push_back(clonedGameObject);
            originalGameObjects.push_back(gameObjectToClone);

            for (UID child : gameObjectToClone->GetChildren())
            {
                gameObjectsToClone.push(std::make_pair(clonedGameObject->GetUID(), child));
            }

            gameObjectToCloneParent->AddChildren(clonedGameObject->GetUID());
            loadedScene->AddGameObject(clonedGameObject->GetUID(), clonedGameObject);
        }

        // ITERATE OVER ALL GAME OBJECTS TO CHECK IF ANY REMAPING IS NEEDED
        for (int i = 0; i < createdGameObjects.size(); ++i)
        {
            MeshComponent* originalMeshComp = originalGameObjects[i]->GetComponent<MeshComponent*>();
            if (originalMeshComp && originalMeshComp->GetHasBones())
            {
                // Remap the bones references
                const std::vector<UID>& bones = originalMeshComp->GetBones();
                std::vector<UID> newBonesUIDs;
                std::vector<GameObject*> newBonesObjects;

                for (const UID bone : bones)
                {
                    const UID uid = remappingTable.find(bone)->second;
                    newBonesUIDs.push_back(uid);
                    newBonesObjects.push_back(loadedScene->GetGameObjectByUID(uid));
                }

                MeshComponent* newMesh = createdGameObjects[i]->GetComponent<MeshComponent*>();
                newMesh->SetBones(newBonesObjects, newBonesUIDs);
            }

            AnimationComponent* animComp = createdGameObjects[i]->GetComponent<AnimationComponent*>();
            if (animComp) animComp->SetBoneMapping();

            // LIGHTS NEED THE INIT AGAIN BECAUSE WHEN CREATING THE COMPONENT THE PARENT GO IS NOT IN THE MAP UNTIL
            // CREATED SO IT DOESEN'T GET ADDED TO THE LIST
            PointLightComponent* pointLight = createdGameObjects[i]->GetComponent<PointLightComponent*>();
            if (pointLight) pointLight->Init();

            DirectionalLightComponent* directionalLight =
                createdGameObjects[i]->GetComponent<DirectionalLightComponent*>();
            if (directionalLight) directionalLight->Init();

            SpotLightComponent* spotLight = createdGameObjects[i]->GetComponent<SpotLightComponent*>();
            if (spotLight) spotLight->Init();
        }
    }

    if (loadedScene->IsMultiselecting())
    {
        const std::map<UID, MobilitySettings> originalObjectMobility = loadedScene->GetMultiselectedObjectsMobility();
        const std::map<UID, float4x4> originalObjectLocals           = loadedScene->GetMultiselectedObjectsLocals();

        for (int i = 0; i < objectsToDuplicate.size(); ++i)
        {
            MobilitySettings originalMobility = originalObjectMobility.find(objectsToDuplicate[i].first)->second;
            const float4x4& ogLocal           = originalObjectLocals.find(objectsToDuplicate[i].first)->second;

            GameObject* currentGameObject =
                loadedScene->GetGameObjectByUID(remappingTable[objectsToDuplicate[i].first]);
            currentGameObject->SetLocalTransform(ogLocal);
            currentGameObject->UpdateMobilityHierarchy(originalMobility);
        }
    }
}

void SceneModule::HandleObjectDeletion()
{
    if (loadedScene->IsMultiselecting()) loadedScene->DeleteMultiselection();
    else loadedScene->RemoveGameObjectHierarchy(loadedScene->GetSelectedGameObjectUID());

    loadedScene->SetSelectedGameObject(loadedScene->GetGameObjectRootUID());
}

void SceneModule::HandleTreesUpdates()
{
    if (loadedScene->IsStaticModified())
    {
        loadedScene->UpdateStaticSpatialStructure();
    }
    if (loadedScene->IsDynamicModified())
    {
        loadedScene->UpdateDynamicSpatialStructure();
    }

    loadedScene->UpdateGameObjects();
}
