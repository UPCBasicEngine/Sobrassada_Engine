#pragma once

#include "Globals.h"
#include "Module.h"
#include "Scene/AABBUpdatable.h"

#include <map>
#include <string>
#include <unordered_map>

class EngineMesh;
class GameObject;
class Component;
class RootComponent;
class Octree;

class SceneModule : public Module
{
  public:
    SceneModule();
    ~SceneModule();

    bool Init() override;
    update_status PreUpdate(float deltaTime) override;
    update_status Update(float deltaTime) override;
    update_status Render(float deltaTime) override;
    update_status RenderEditor(float deltaTime) override;
    update_status PostUpdate(float deltaTime) override;
    bool ShutDown() override;

    void LoadScene(UID sceneUID, const char *sceneName, UID rootGameObject);
    void CloseScene();

    void RenderHierarchyUI(bool &hierarchyMenu);
    void RenderGameObjectHierarchy(UID gameObjectUUID);

    void HandleNodeClick(UID gameObjectUUID);
    void RenderContextMenu(UID gameObjectUUID);

    void RemoveGameObjectHierarchy(UID gameObjectUUID);
    void UpdateGameObjectHierarchy(UID sourceUUID, UID targetUUID);

	//TODO: Change when filesystem defined
    GameObject *GetGameObjectByUUID(UID gameObjectUUID) { return gameObjectsContainer[gameObjectUUID]; }

    GameObject *GetSeletedGameObject() { return GetGameObjectByUUID(selectedGameObjectUUID); }

    const std::unordered_map<UID, GameObject *> &GetAllGameObjects() const { return gameObjectsContainer; }
    const std::map<UID, Component *> &GetAllComponents() const { return gameComponents; }
    UID GetGameObjectRootUID() const { return gameObjectRootUUID; }

    std::map<UID, Component*> gameComponents;

    AABBUpdatable* GetTargetForAABBUpdate(UID uuid);

    UID GetSceneUID() const { return sceneUID; }
    const std::string &GetSceneName() const { return sceneName; }

    void AddGameObject(UID uid, GameObject *newGameObject) { gameObjectsContainer.insert({uid, newGameObject}); }
    void AddComponent(UID uid, Component *newComponent) { gameComponents.insert({uid, newComponent}); }

    

  private:
    void CreateSpatialDataStruct();
    void UpdateSpatialDataStruct();
    void CheckObjectsToRender(std::vector<GameObject *> &outRenderGameObjects) const;

  private:
    UID sceneUID;
    UID gameObjectRootUUID;
    std::string sceneName;
    
    UID selectedGameObjectUUID;

    std::unordered_map<UID, GameObject*> gameObjectsContainer; //For testing purposes until FileSystem available

    Octree *sceneOctree = nullptr;

    // Scene* loadedScene = nullptr;

    // pawnClassType;
    // updatedGameObjects;
    // sceneSpatialDataStruct;
    // inputClassType;
    // gameConfigClassType;

    // lightsConfig;
};
