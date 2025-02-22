﻿#pragma once

#include "Globals.h"
#include "LightsConfig.h"

#include <map>
#include <unordered_map>

class GameObject;
class Component;
class RootComponent;
class AABBUpdatable;
class Octree;

class Scene
{
  public:
    Scene(UID sceneUID, const char* sceneName, UID rootGameObject);

    ~Scene();

    void Save() const;

    void LoadComponents(const std::map<UID, Component*>& loadedGameComponents);
    void LoadGameObjects(const std::unordered_map<UID, GameObject*>& loadedGameObjects);

    update_status Render(float deltaTime);
    update_status RenderEditor(float deltaTime);
    void RenderScene();
    void RenderSelectedGameObjectUI();


    void RenderHierarchyUI(bool& hierarchyMenu);

    void RemoveGameObjectHierarchy(UID gameObjectUUID);

    // TODO: Change when filesystem defined
    GameObject* GetGameObjectByUUID(UID gameObjectUUID);
    Component* GetComponentByUID(UID componentUID);

    GameObject* GetSeletedGameObject() { return GetGameObjectByUUID(selectedGameObjectUUID); }

    const std::unordered_map<UID, GameObject*>& GetAllGameObjects() const { return gameObjectsContainer; }
    const std::map<UID, Component*>& GetAllComponents() const { return gameComponents; }
    UID GetGameObjectRootUID() const { return gameObjectRootUUID; }

    void RemoveComponent(UID componentUID);

    AABBUpdatable* GetTargetForAABBUpdate(UID uuid);

    UID GetSceneUID() const { return sceneUID; }
    const char* GetSceneName() const { return sceneName.c_str(); }

    void AddGameObject(UID uid, GameObject* newGameObject) { gameObjectsContainer.insert({uid, newGameObject}); }
    void AddComponent(UID uid, Component* newComponent) { gameComponents.insert({uid, newComponent}); }

    LightsConfig* GetLightsConfig() { return lightsConfig; }
    
    void UpdateSpatialDataStruct();

  private:
    void CreateSpatialDataStruct();
    void CheckObjectsToRender(std::vector<GameObject*>& outRenderGameObjects) const;

  private:
    std::string sceneName;
    UID sceneUID;
    UID gameObjectRootUUID;

    UID selectedGameObjectUUID;

    std::map<UID, Component*> gameComponents; // TODO Move components to individual gameObjects
    std::unordered_map<UID, GameObject*> gameObjectsContainer;

    LightsConfig* lightsConfig = nullptr;
    Octree* sceneOctree        = nullptr;
};
