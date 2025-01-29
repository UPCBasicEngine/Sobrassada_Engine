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
    //void RenderGameObjectHierarchy(uint32_t gameObjectUUID, uint32_t &selectedGameObjectUUID);

private:

	uint32_t gameObjectRootUUID;

	std::unordered_map<uint32_t, GameObject*> gameObjectsContainer; //For testing purposes
	
	/*bool hierarchyMenu = false;*/
	//pawnClassType;
	//updatedGameObjects;
	//sceneSpatialDataStruct;
	//inputClassType;
	//gameConfigClassType;

	//lightsConfig;


};
