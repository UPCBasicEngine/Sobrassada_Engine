#pragma once

#include "Globals.h"
#include "HashString.h"
#include "LightsConfig.h"

#include "Math/float3x4.h"
#include "Math/float4x4.h"
#include <functional>
#include <map>
#include <set>
#include <tuple>
#include <unordered_map>
#include <vector>

class WindConfig;
class GameObject;
class Component;
class RootComponent;
class Octree;
class ResourcePrefab;
class Quadtree;
class CameraComponent;
class FrustumPlanes;
class CharacterControllerComponent;
class Framebuffer;
class RenderPass;
enum class SaveMode;
enum MobilitySettings;

class SOBRASADA_API_ENGINE Scene
{
  public:
    Scene(const char* sceneName);
    Scene(const rapidjson::Value& initialState, UID loadedSceneUID);

    ~Scene();

    void Init();
    void Save(
        rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator, SaveMode saveMode,
        UID newUID = INVALID_UID, const char* newName = nullptr
    );

    void LoadModel(const UID modelUID);
    void LoadPrefab(
        const UID prefabUID, const ResourcePrefab* prefab = nullptr, const float4x4& transform = float4x4::identity,
        bool isEnabled = true, std::vector<bool> componentsEnabledStates = {},
        const HashString& assignTag = HashString("")
    );
    void OverridePrefabs(UID prefabUID);

    update_status Update(float deltaTime);
    update_status PostUpdate(float deltaTime);
    update_status Render(float deltaTime);
    update_status RenderEditor(float deltaTime);

    void RenderEditorControl(bool& editorControlMenu);
    void RenderScene(float deltaTime, CameraComponent* camera);
    void RenderSceneToFrameBuffer();
    void RenderSelectedGameObjectUI();
    void RenderHierarchyUI(bool& hierarchyMenu);

    bool IsStaticModified() const { return staticModified; };
    bool IsDynamicModified() const { return dynamicModified; };
    bool IsMultiselecting() const { return selectedGameObjects.size() > 0; };
    bool IsSceneFocused() const { return isFocused; };

    void UpdateStaticSpatialStructure();
    void UpdateDynamicSpatialStructure();

    void AddGameObject(UID uid, GameObject* newGameObject) { gameObjectsContainer.insert({uid, newGameObject}); }
    void RemoveGameObjectHierarchy(UID gameObjectUUID);

    void AddGameObjectToUpdateComponents(GameObject* gameObject);
    void UpdateGameObjectsComponents();
    void ClearGameObjectsToUpdateComponents();

    void AddGameObjectToSelection(UID gameObject, UID gameObjectParent);
    void ClearObjectSelection();
    void DeleteMultiselection();

    void CreateTag(HashString&& newTag);
    void DeleteTag(const HashString& tagToDelete);
    void RequestTag(const HashString& requestTag, GameObject* gameObject);
    void RemoveFromTag(const HashString& requestTag, GameObject* gameObject);
    const std::vector<GameObject*>* GetTaggedGameObjects(const HashString& requestTag);
    const std::map<HashString, std::vector<GameObject*>>& GetTags() { return tags; };

    const std::string& GetSceneName() const { return sceneName; }
    UID GetSceneUID() const { return sceneUID; }
    UID GetGameObjectRootUID() const { return gameObjectRootUID; }
    GameObject* GetSelectedGameObject() { return GetGameObjectByUID(selectedGameObjectUID); }
    UID GetSelectedGameObjectUID() const { return selectedGameObjectUID; }

    const std::unordered_map<UID, GameObject*>& GetAllGameObjects() const { return gameObjectsContainer; }

    GameObject* GetGameObjectByUID(UID gameObjectUID); // TODO: Change when filesystem defined
    GameObject* GetGameObjectByName(const std::string& name);
    GameObject* GetGameObjectByParentNameAndTargetName(const std::string& parentName, const std::string& targetName);

    LightsConfig* GetLightsConfig() const { return lightsConfig; }
    WindConfig* GetWindsConfig() const { return windConfig; }
    CameraComponent* GetMainCamera() const;
    CameraComponent* GetMainCameraEvenDisabled() const;

    template <typename T> std::vector<T> GetEnabledComponentsOfType() const;

    bool GetDoInputs() const { return doInputs; }
    bool GetDoMouseInputs() const { return doMouseInputs; }
    bool GetStartPlaying() const { return startPlaying; }
    bool GetStepPlaying() const { return stepPlaying; }
    bool GetStopPlaying() const { return stopPlaying; }
    bool GetSceneVisible() const { return sceneVisible; }

    const std::tuple<float, float>& GetWindowPosition() const { return sceneWindowPosition; };
    const std::tuple<float, float>& GetWindowSize() const { return sceneWindowSize; };
    const std::tuple<float, float>& GetMousePosition() const { return mousePosition; };
    Octree* GetOctree() const { return sceneOctree; }
    Octree* GetDynamicTree() const { return dynamicTree; }
    UID GetMultiselectUID() const;
    GameObject* GetMultiselectParent() const { return multiSelectParent; }
    UID GetNavmeshUID() const { return navmeshUID; }
    GameObject* GetMultiselectParent() { return multiSelectParent; }
    const std::map<UID, UID>& GetMultiselectedObjects() const { return selectedGameObjects; }
    const std::map<UID, MobilitySettings>& GetMultiselectedObjectsMobility() const
    {
        return selectedGameObjectsMobility;
    }
    const std::map<UID, float4x4>& GetMultiselectedObjectsLocals() const { return selectedGameObjectsOgLocals; }

    void SetMainCamera(CameraComponent* camera) { mainCamera = camera; }
    void SetSelectedGameObject(UID newSelectedGameObject) { selectedGameObjectUID = newSelectedGameObject; };
    void SetNavmeshUID(UID navUID) { navmeshUID = navUID; }
    void SetStartPlaying(bool start) { startPlaying = start; }
    void SetStepPlaying(bool step) { stepPlaying = step; }
    void SetStopPlaying(bool stop) { stopPlaying = stop; }

    void SetStaticModified() { staticModified = true; }
    void SetDynamicModified() { dynamicModified = true; }
    void SetMultiselectPosition(const float3& newPosition);

    void CheckObjectsToUpdate();
    void ClearObjectsToUpdate();
    void CheckObjectsInFrustum(std::vector<GameObject*>& outOpaqueRenderGameObjects, FrustumPlanes frustumPlanes) const;
    const HashString& GetPlayerLocation() { return playerLocation; }
    void SetPlayerPosition(const HashString& newPlayerLocation) { playerLocation = newPlayerLocation; }

    void UpdateAllMaterialInstances(const UID materialUID);

    void QueueGameObjectDelete(UID uid);
    void FlushPendingDeletes();

    bool isSceneLoaded = false;

  private:
    void CreateStaticSpatialDataStruct();
    void CreateDynamicSpatialDataStruct();

  private:
    std::string sceneName       = DEFAULT_SCENE_NAME;
    UID sceneUID                = INVALID_UID;
    UID navmeshUID              = INVALID_UID;
    UID gameObjectRootUID       = INVALID_UID;
    UID selectedGameObjectUID   = INVALID_UID;
    CameraComponent* mainCamera = nullptr;
    bool startPlaying           = false;
    bool stepPlaying            = false;
    bool stopPlaying            = false;
    bool doInputs               = false;
    bool doMouseInputs          = false;
    bool sceneVisible           = false;
    bool isFocused              = false;

    std::unordered_map<UID, GameObject*> gameObjectsContainer;

    LightsConfig* lightsConfig                   = nullptr;
    WindConfig* windConfig                       = nullptr;
    Octree* sceneOctree                          = nullptr;
    Octree* dynamicTree                          = nullptr;

    // IMGUI WINDOW DATA
    std::tuple<float, float> sceneWindowPosition = std::make_tuple(0.f, 0.f);
    std::tuple<float, float> sceneWindowSize     = std::make_tuple(0.f, 0.f);
    std::tuple<float, float> mousePosition       = std::make_tuple(0.f, 0.f);

    bool staticModified                          = false;
    bool dynamicModified                         = false;

    std::vector<GameObject*> gameObjectsToUpdateComponents;

    std::set<UID> toUpdateGameObjectsSet;
    std::vector<GameObject*> toUpdateGameObjects;

    GameObject* multiSelectParent = nullptr;
    std::map<UID, UID> selectedGameObjects;
    std::map<UID, MobilitySettings> selectedGameObjectsMobility;
    std::map<UID, float4x4> selectedGameObjectsOgLocals;

    std::unordered_map<uint64_t, const rapidjson::Value*> gameObjectDataMap;

    HashString emptyString    = HashString("");
    HashString playerLocation = HashString("");
    std::map<HashString, std::vector<GameObject*>> tags;

    RenderPass* renderPass = nullptr;

     std::vector<UID> pendingDeletes;
};
