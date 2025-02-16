#include "Scene.h"

#include "Application.h"
#include "CameraModule.h"
#include "Component.h"
#include "Framebuffer.h"
#include "GameObject.h"
#include "OpenGLModule.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "./Libs/ImGuizmo/ImGuizmo.h"

Scene::Scene(UID sceneUID, const char *sceneName, UID rootGameObject) : sceneUID(sceneUID), sceneName(sceneName), gameObjectRootUUID(rootGameObject)
{
    selectedGameObjectUUID = gameObjectRootUUID;
    
    lightsConfig           = new LightsConfig();
    lightsConfig->InitSkybox();
    lightsConfig->InitLightBuffers();
}

Scene::~Scene()
{
    for (auto it = gameObjectsContainer.begin(); it != gameObjectsContainer.end(); ++it)
    {
        delete it->second;
    }
    gameObjectsContainer.clear();

    for (auto it = gameComponents.begin(); it != gameComponents.end(); ++it)
    {
        delete it->second;
    }
    gameComponents.clear();

    delete lightsConfig;
    lightsConfig = nullptr;

    GLOG("%s scene closed", sceneName)
}

void Scene::Load(const std::map<UID, Component*>& loadedGameComponents, const std::unordered_map<UID, GameObject*>& loadedGameObjects)
{
    gameComponents.clear();
    gameObjectsContainer.clear();
    gameComponents.insert(loadedGameComponents.begin(), loadedGameComponents.end());
    gameObjectsContainer.insert(loadedGameObjects.begin(), loadedGameObjects.end());
    
    GameObject* root = GetGameObjectByUUID(gameObjectRootUUID);
    if (root != nullptr)
    {
        GLOG("Init transform and AABB calculation")
        root->ComponentGlobalTransformUpdated();
    }
}

update_status Scene::Render(float deltaTime)
{
    // Render skybox and lights
    lightsConfig->RenderSkybox();
    lightsConfig->SetLightsShaderData();

    for (auto it = gameObjectsContainer.begin(); it != gameObjectsContainer.end(); ++it)
    {
        if (it->second != nullptr)
        {
            it->second->Render();
        }
        else
        {
            GLOG("Empty gameObject in scene detected")
        }
    }
    return UPDATE_CONTINUE;
}

update_status Scene::RenderEditor(float deltaTime)
{
    if (ImGui::Begin(sceneName))
    {
	    
        if (ImGui::BeginChild("##SceneChild", ImVec2(0.f, 0.f), NULL, ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar))
        {
            const auto& framebuffer = App->GetOpenGLModule()->GetFramebuffer();

            ImGui::SetCursorPos(ImVec2(0.f, 0.f));

            ImGui::Image(
                    (ImTextureID)framebuffer->GetTextureID(),
                    ImVec2((float)framebuffer->GetTextureWidth(), (float)framebuffer->GetTextureHeight()),
                    ImVec2(0.f, 1.f),
                    ImVec2(1.f, 0.f)
            );

            ImGuizmo::SetOrthographic(false);
            ImGuizmo::SetDrawlist(); // ImGui::GetWindowDrawList()

            float width = ImGui::GetWindowWidth();
            float height = ImGui::GetWindowHeight();
            ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, width, height);
		    
            ImGui::EndChild();

            ImVec2 windowSize = ImGui::GetWindowSize();
            if (framebuffer->GetTextureWidth() != windowSize.x || framebuffer->GetTextureHeight() != windowSize.y)
            {
                float aspectRatio = windowSize.y / windowSize.x;
                App->GetCameraModule()->SetAspectRatio(aspectRatio);
                framebuffer->Resize((int)windowSize.x, (int)windowSize.y);
            }
        }
        ImGui::End();
    }

    GameObject* selectedGameObject = gameObjectsContainer[selectedGameObjectUUID];
    if (selectedGameObject != nullptr)
    {
        selectedGameObject->RenderEditor();
    }
    return UPDATE_CONTINUE;
}

void Scene::RenderHierarchyUI(bool& hierarchyMenu)
{
    ImGui::Begin("Hierarchy", &hierarchyMenu);

    if (ImGui::Button("Add GameObject"))
    {
        GameObject* newGameObject = new GameObject(selectedGameObjectUUID, "new Game Object");
        UID newUUID               = newGameObject->GetUID();

        GetGameObjectByUUID(selectedGameObjectUUID)->AddGameObject(newUUID);

        // TODO: change when filesystem defined
        gameObjectsContainer.insert({newUUID, newGameObject});

        GetGameObjectByUUID(newGameObject->GetParent())->ComponentGlobalTransformUpdated();
    }

    if (selectedGameObjectUUID != gameObjectRootUUID)
    {
        ImGui::SameLine();

        if (ImGui::Button("Delete GameObject"))
        {
            UID parentUID                = GetGameObjectByUUID(selectedGameObjectUUID)->GetParent();
            GameObject* parentGameObject = GetGameObjectByUUID(parentUID);
            RemoveGameObjectHierarchy(selectedGameObjectUUID);
            // parentGameObject->PassAABBUpdateToParent(); // TODO: check if it works
        }
    }

    GameObject* rootGameObject = GetGameObjectByUUID(gameObjectRootUUID);
    if (rootGameObject)
    {
        rootGameObject->RenderHierarchyNode(selectedGameObjectUUID);
    }

    ImGui::End();
}

void Scene::RemoveGameObjectHierarchy(UID gameObjectUUID)
{
    // TODO: Change when filesystem defined
    if (!gameObjectsContainer.count(gameObjectUUID) || gameObjectUUID == gameObjectRootUUID) return;

    GameObject* gameObject = GetGameObjectByUUID(gameObjectUUID);

    for (UID childUUID : gameObject->GetChildren())
    {
        RemoveGameObjectHierarchy(childUUID);
    }

    UID parentUUID = gameObject->GetParent();

    // TODO: change when filesystem defined
    if (gameObjectsContainer.count(parentUUID))
    {
        GameObject* parentGameObject = GetGameObjectByUUID(parentUUID);
        parentGameObject->RemoveGameObject(gameObjectUUID);
        selectedGameObjectUUID = parentUUID;
    }

    // TODO: change when filesystem defined
    gameObjectsContainer.erase(gameObjectUUID);

    delete gameObject;
}

void Scene::RemoveComponent(uint64_t componentUID){
    gameComponents.erase(componentUID);
}

AABBUpdatable* Scene::GetTargetForAABBUpdate(UID uuid)
{
    if (gameObjectsContainer.count(uuid))
    {
        return gameObjectsContainer[uuid];
    }

    if (gameComponents.count(uuid))
    {
        return gameComponents[uuid];
    }

    return nullptr;
}

GameObject* Scene::GetGameObjectByUUID(UID gameObjectUUID)
{
    if (gameObjectsContainer.count(gameObjectUUID))
    {
        return gameObjectsContainer[gameObjectUUID];
    }
    return nullptr;
}

Component* Scene::GetComponentByUID(uint64_t componentUID)
{
    if (gameComponents.count(componentUID))
    {
        return gameComponents[componentUID];
    }
    return nullptr;
}
