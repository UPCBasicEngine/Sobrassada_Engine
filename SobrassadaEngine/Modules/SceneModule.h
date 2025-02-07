#pragma once

#include "Module.h"

#include <map>
#include <string>
#include <unordered_map>


class EngineModel;class GameObject;
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
    void RenderGameObjectHierarchy(uint32_t gameObjectUUID, uint32_t &selectedGameObjectUUID);

    std::map<uint32_t, Component*> gameComponents;
    std::map<uint32_t, EngineModel*> loadedModels;
    std::map<std::string, uint32_t> MOCKUP_libraryModels;
    
    RootComponent* GetRootComponent() const { return rootComponent; }

    void MOCKUP_loadModel(std::string path);

private:

	uint32_t gameObjectRootUUID;
        RootComponent *rootComponent;
    

	std::unordered_map<uint32_t, GameObject*> gameObjectsContainer; //For testing purposes until FileSystem available
	
	/*bool hierarchyMenu = false;*/
	//pawnClassType;
	//updatedGameObjects;
	//sceneSpatialDataStruct;
	//inputClassType;
	//gameConfigClassType;

	//lightsConfig;


};
