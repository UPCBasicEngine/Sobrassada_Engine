#pragma once

#include "EngineModel.h"
#include "Module.h"

#include <map>
#include <string>
#include <unordered_map>


class EngineMesh
;class GameObject;
class Component;
class RootComponent;

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

	void CreateSpatialDataStruct();
	void UpdateSpatialDataStruct();

	void CheckObjectsToRender();
	
	void RenderHierarchyUI(bool &hierarchyMenu);
    void RenderGameObjectHierarchy(uint32_t gameObjectUUID);

	void HandleNodeClick(uint32_t gameObjectUUID);
    void RenderContextMenu(uint32_t gameObjectUUID);

	void RemoveGameObjectHierarchy(uint32_t gameObjectUUID);
    void UpdateGameObjectHierarchy(uint32_t sourceUUID, uint32_t targetUUID);

	//TODO: Change when filesystem defined
	inline GameObject *GetGameObjectByUUID(uint32_t gameObjectUUID) { return gameObjectsContainer[gameObjectUUID]; }

    GameObject * GetSeletedGameObject() { return GetGameObjectByUUID(selectedGameObjectUUID); }


    std::map<uint32_t, Component*> gameComponents;
    std::map<uint32_t, EngineMesh*> MOCKUP_loadedMeshes;
    std::map<std::string, uint32_t> MOCKUP_libraryMeshes;
    EngineModel* MOCKUP_loadedModel;

    void MOCKUP_loadModel(std::string path);

private:

	uint32_t gameObjectRootUUID;
    
	uint32_t selectedGameObjectUUID;

	std::unordered_map<uint32_t, GameObject*> gameObjectsContainer; //For testing purposes until FileSystem available
    
	//pawnClassType;
	//updatedGameObjects;
	//sceneSpatialDataStruct;
	//inputClassType;
	//gameConfigClassType;

	//lightsConfig;


};
