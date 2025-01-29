#include "SceneModule.h"
#include "GameObject.h"

#include "glew.h"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl2.h"

SceneModule::SceneModule()
{
}

SceneModule::~SceneModule()
{
}

bool SceneModule::Init()
{
    GLOG("Creating SceneModule renderer context");

    GameObject* sceneGameObject = new GameObject("SceneModule");
    gameObjectRootUUID = 0000001;

    gameObjectsContainer.insert({gameObjectRootUUID, sceneGameObject});

    return true;
}

update_status SceneModule::PreUpdate(float deltaTime) 
{
    return UPDATE_CONTINUE;
}

update_status SceneModule::Update(float deltaTime) 
{
    return UPDATE_CONTINUE;
}

update_status SceneModule::PostUpdate(float deltaTime)
{
    return UPDATE_CONTINUE;
}

bool SceneModule::ShutDown()
{
    GLOG("Destroying renderer");
    return true;
}

void SceneModule::LoadScene()
{
}

void SceneModule::CloseScene()
{
}

void SceneModule::CreateSpatialDataStruct()
{
}

void SceneModule::UpdateSpatialDataStruct()
{
}

void SceneModule::CheckObjectsToRender()
{
}

void SceneModule::RenderHierarchyUI(bool &hierarchyMenu) 
{ 
    ImGui::Begin("Hierarchy", &hierarchyMenu);

    if(ImGui::Button("Add GameObject"));
    
    ImGui::SameLine();

    if(ImGui::Button("Delete GameObject"));
    //Make basic implementation and remember to call the function
    //instance of moduleScene is required

    //Put a button to add and remove gameobject

    ImGui::End();
}