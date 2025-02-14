#pragma once

#include "EngineModel.h"
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

    void LoadScene();
    void CloseScene();

    void RenderHierarchyUI(bool &hierarchyMenu);
    void RenderGameObjectHierarchy(uint32_t gameObjectUUID);

    void HandleNodeClick(uint32_t gameObjectUUID);
    void RenderContextMenu(uint32_t gameObjectUUID);

    void RemoveGameObjectHierarchy(uint32_t gameObjectUUID);
    void UpdateGameObjectHierarchy(uint32_t sourceUUID, uint32_t targetUUID);

    // TODO: Change when filesystem defined
    GameObject *GetGameObjectByUUID(uint32_t gameObjectUUID) { return gameObjectsContainer[gameObjectUUID]; }

    GameObject *GetSeletedGameObject() { return GetGameObjectByUUID(selectedGameObjectUUID); }

    std::map<uint32_t, Component *> gameComponents;

    std::map<uint32_t, EngineMesh *> MOCKUP_loadedMeshes;
    std::map<std::string, uint32_t> MOCKUP_libraryMeshes;

    std::map<uint32_t, unsigned int> MOCKUP_loadedTextures;
    std::map<std::string, uint32_t> MOCKUP_libraryTextures;

    EngineModel *MOCKUP_loadedModel;

    AABBUpdatable *GetTargetForAABBUpdate(uint32_t uuid);

    void MOCKUP_loadModel(std::string path);

  private:
    void CreateSpatialDataStruct();
    void UpdateSpatialDataStruct();
    void CheckObjectsToRender(std::vector<GameObject *> &outRenderGameObjects) const;

    // SPATIAL_PARTITIONING TESTING
    void CreateHouseGameObject(int totalGameobjects);
    void RenderBoundingBoxes();
    void RenderOctree();

  private:
    uint32_t gameObjectRootUUID;

    uint32_t selectedGameObjectUUID;

    std::unordered_map<uint32_t, GameObject *> gameObjectsContainer; // For testing purposes until FileSystem available

    Octree *sceneOctree = nullptr;

    // Scene* loadedScene = nullptr;

    // pawnClassType;
    // updatedGameObjects;
    // sceneSpatialDataStruct;
    // inputClassType;
    // gameConfigClassType;

    // lightsConfig;
};
