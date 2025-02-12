#pragma once

#include "Globals.h"
#include "EngineModel.h"
#include "Module.h"
#include "Scene/AABBUpdatable.h"

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
    void RenderGameObjectHierarchy(UID gameObjectUUID);

	void HandleNodeClick(UID gameObjectUUID);
    void RenderContextMenu(UID gameObjectUUID);

	void RemoveGameObjectHierarchy(UID gameObjectUUID);
    void UpdateGameObjectHierarchy(UID sourceUUID, UID targetUUID);

	//TODO: Change when filesystem defined
    GameObject *GetGameObjectByUUID(UID gameObjectUUID) { return gameObjectsContainer[gameObjectUUID]; }

    GameObject * GetSeletedGameObject() { return GetGameObjectByUUID(selectedGameObjectUUID); }


    std::map<UID, Component*> gameComponents;
    
    std::map<UID, EngineMesh*> MOCKUP_loadedMeshes;
    std::map<std::string, UID> MOCKUP_libraryMeshes;

    std::map<UID, unsigned int> MOCKUP_loadedTextures;
    std::map<std::string, UID> MOCKUP_libraryTextures;
    
    EngineModel* MOCKUP_loadedModel;

    AABBUpdatable* GetTargetForAABBUpdate(UID uuid);

    void MOCKUP_loadModel(std::string path);

private:

	UID gameObjectRootUUID;
    
	UID selectedGameObjectUUID;

	std::unordered_map<UID, GameObject*> gameObjectsContainer; //For testing purposes until FileSystem available

    // Scene* loadedScene = nullptr;
    
	//pawnClassType;
	//updatedGameObjects;
	//sceneSpatialDataStruct;
	//inputClassType;
	//gameConfigClassType;

	//lightsConfig;


};
