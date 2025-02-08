#pragma once
#include "Module.h"

#include <unordered_map>

class GameObject;

class SceneModule : public Module
{
public:

	SceneModule();
	~SceneModule();

	bool Init() override;
    update_status PreUpdate(float deltaTime) override;
    update_status Update(float deltaTime) override;
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
