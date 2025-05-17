#pragma once

#include "Application.h"
#include "ComponentUtils.h"
#include "Globals.h"
#include "SceneModule.h"

#include "Geometry/AABB.h"
#include "Geometry/OBB.h"
#include "rapidjson/document.h"
#include <bitset>
#include <queue>
#include <string>
#include <tuple>
#include <vector>

class MeshComponent;
class PointLightComponent;
class SpotLightComponent;
class DirectionalLightComponent;
class CharacterControllerComponent;
class Transform2DComponent;
class CanvasComponent;
class UILabelComponent;
class CameraComponent;
class ScriptComponent;
class CubeColliderComponent;
class SphereColliderComponent;
class CapsuleColliderComponent;
class AnimationComponent;
class AIAgentComponent;
class ImageComponent;
class ButtonComponent;
class AudioSourceComponent;
class AudioListenerComponent;

enum MobilitySettings
{
    DYNAMIC = 0,
    STATIC  = 1,
};

class SOBRASADA_API_ENGINE GameObject
{
  public:
    GameObject(const std::string& name);
    GameObject(UID parentUID, const std::string& name);
    GameObject(UID parentUID, const std::string& name, UID uid);
    GameObject(UID parentUID, GameObject* refObject);

    GameObject(const rapidjson::Value& initialState);

    ~GameObject();

    void LoadData(const rapidjson::Value& initialState);

    void Init();
    void InitHierarchy();

    const float4x4& GetParentGlobalTransform() const;

    bool IsStatic() const { return mobilitySettings == MobilitySettings::STATIC; };
    bool HasSelectParent() const { return selectParent; };
    bool IsNavMeshValid() const { return navMeshValid; };
    bool IsComponentCreated(int i) const { return createdComponents[i]; };

    bool AddGameObject(UID gameObjectUID);
    bool RemoveGameObject(UID gameObjectUID);

    void Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const;

    void UpdateEnabledStateRecursive();
    void RenderHierarchyNode(UID& selectedGameObjectUID);
    void HandleNodeClick(UID& selectedGameObjectUID);
    void RenderContextMenu();
    void RenderEditorInspector();

    void UpdateGameObjectHierarchy(UID sourceUID);
    void RenameGameObjectHierarchy();
    bool TargetIsChildren(UID uidTarget);
    void UpdateComponents(float deltaTime);
    void RenderDebugComponents(float deltaTime);

    const std::string& GetName() const { return name; }
    void SetName(const std::string& newName) { name = newName; }

    const std::vector<UID>& GetChildren() const { return children; }
    void AddChildren(UID childUID) { children.push_back(childUID); }

    UID GetParent() const { return parentUID; }
    void SetParent(UID newParentUID) { parentUID = newParentUID; }

    UID GetUID() const { return uid; }
    void SetUID(UID newUID) { uid = newUID; }

    const AABB& GetLocalAABB() const { return localAABB; }

    const AABB& GetGlobalAABB() const { return globalAABB; }
    const OBB& GetGlobalOBB() const { return globalOBB; }

    void OnAABBUpdated();

    void Render(float deltatime) const;
    void RenderEditor();

    const float4x4& GetGlobalTransform() const { return globalTransform; }
    const float4x4& GetLocalTransform() const { return localTransform; }

    bool CreateComponent(ComponentType componentType);
    bool RemoveComponent(ComponentType componentType);

    // Updates the transform for this game object and all descending children
    void UpdateTransformForGOBranch();
    void UpdateMobilityHierarchy(MobilitySettings type);
    void UpdateLocalTransform(const float4x4& parentGlobalTransform);
    void UpdateOpenNodeHierarchy(bool openValue);

    bool WillUpdate() const { return willUpdate; };

    template <typename T> T GetComponent() const { return std::get<T>(compTuple); }
    template <typename T> T GetComponentChild(Application* app) const;
    template <typename T> T GetComponentParent(Application* app) const;

    const float3& GetPosition() const { return position; }
    const float3& GetRotation() const { return rotation; }
    const float3& GetScale() const { return scale; }
    AABB GetHierarchyAABB();
    std::tuple<COMPONENTS>& GetComponentsTupleRef() { return compTuple; }
    const bool HasScriptsToLoad() const { return hasScriptsToLoad; }

    void SetLocalTransform(const float4x4& newTransform);
    void SetLocalPosition(const float3& newPos);
    void DrawGizmos() const;

    void CreatePrefab();
    bool IsGloballyEnabled() const;
    UID GetPrefabUID() const { return prefabUID; }
    void SetPrefabUID(const UID uid) { prefabUID = uid; }
    void ParentUpdatedComponents();
    void OnTransformUpdated();
    void SetPosition(float3& newPosition) { position = newPosition; };
    void SetWillUpdate(bool willUpdate) { this->willUpdate = willUpdate; };
    bool IsEnabled() const { return enabled; }
    void SetEnabled(bool state) { enabled = state; }
    void SetComponentCreated(int position) { createdComponents[position] = true; }
    void SetComponentRemoved(int position) { createdComponents[position] = false; }
    void SetSelectParent(bool newSelectParent) { selectParent = newSelectParent; }
    void SetOpenHierarchyNode(bool newOpen) { openHierarchyNode = newOpen; }

  private:
    void DrawNodes() const;
    void OnDrawConnectionsToggle();

    void SetMobility(MobilitySettings newMobility) { mobilitySettings = newMobility; };

  public:
    inline static UID currentRenamingUID = INVALID_UID;

  private:
    UID parentUID;
    UID uid;
    std::vector<UID> children;

    std::string name = "";

    AABB localAABB;
    AABB globalAABB;
    OBB globalOBB;

    bool isRenaming = false;
    char renameBuffer[128];

    UID prefabUID                        = INVALID_UID;
    bool drawNodes                       = false;

    float3 position                      = float3::zero;
    float3 rotation                      = float3::zero;
    float3 scale                         = float3::one;

    float4x4 localTransform              = float4x4::identity;
    float4x4 globalTransform             = float4x4::identity;

    ComponentType selectedComponentIndex = COMPONENT_NONE;
    int mobilitySettings                 = STATIC;
    bool selectParent                    = false;
    bool willUpdate                      = false;
    bool enabled                         = true;
    bool wasEnabled                      = true;
    bool navMeshValid                    = false;
    bool openHierarchyNode               = false;

    std::tuple<COMPONENTS> compTuple     = std::make_tuple(COMPONENTS_NULLPTR);
    std::bitset<std::tuple_size<decltype(compTuple)>::value> createdComponents;

    bool hasScriptsToLoad = false;
};

template <typename T> inline T GameObject::GetComponentChild(Application* app) const
{
    T component = nullptr;

    std::queue<UID> gameObjects;

    for (UID child : this->GetChildren())
    {
        gameObjects.push(child);
    }

    Scene* scene = app->GetSceneModule()->GetScene();

    while (!gameObjects.empty())
    {
        UID currentGameObject = gameObjects.front();
        gameObjects.pop();

        GameObject* current = scene->GetGameObjectByUID(currentGameObject);
        component           = current->GetComponent<T>();

        if (component) break;

        for (UID child : current->GetChildren())
        {
            gameObjects.push(child);
        }
    }

    return component;
}

template <typename T> inline T GameObject::GetComponentParent(Application* app) const
{
    T component    = nullptr;
    UID currentUID = parentUID;

    Scene* scene   = app->GetSceneModule()->GetScene();

    while (currentUID != scene->GetGameObjectRootUID())
    {
        GameObject* current = scene->GetGameObjectByUID(currentUID);
        component           = current->GetComponent<T>();

        if (component) break;

        currentUID = current->parentUID;
    }

    return component;
}
