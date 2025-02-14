#include "LibraryModule.h"

#include "Application.h"
#include "Component.h"
#include "ComponentUtils.h"
#include "FileSystem.h"
#include "GameObject.h"
#include "Root/RootComponent.h"
#include "SceneImporter.h"
#include "SceneModule.h"
#include "document.h"
#include "prettywriter.h"
#include "stringbuffer.h"

#include <filesystem>

LibraryModule::LibraryModule() {}

LibraryModule::~LibraryModule() {}

bool LibraryModule::Init()
{
    SceneImporter::CreateLibraryDirectories();
    LibraryModule::LoadLibraryMaps();

    return true;
}

// Save = library/scenes/
// SaveAs = all path + name.scene
bool LibraryModule::SaveScene(const char *path, SaveMode saveMode) const
{
    SceneModule *sceneModule = App->GetSceneModule();
    if (sceneModule == nullptr)
    {
        GLOG("Scene module not found");
        return false;
    }

    UID sceneUID = (saveMode == SaveMode::Save) ? sceneModule->GetSceneUID() : GenerateUID();
    const std::string &sceneName =
        (saveMode == SaveMode::Save) ? sceneModule->GetSceneName() : FileSystem::GetFileNameWithoutExtension(path);

    if (sceneUID == 0 && saveMode == SaveMode::Save) return false;

    UID gameObjectRootUID   = sceneModule->GetGameObjectRootUID();

    const auto &gameObjects = sceneModule->GetAllGameObjects();
    const auto &components  = sceneModule->GetAllComponents();

    // Create doc JSON
    rapidjson::Document doc;
    doc.SetObject();
    rapidjson::Document::AllocatorType &allocator = doc.GetAllocator();

    rapidjson::Value scene(rapidjson::kObjectType);

    // Scene values
    UID uid                 = sceneUID;
    const std::string &name = sceneName;
    UID rootGameObject      = gameObjectRootUID;

    // Create structure
    scene.AddMember("UID", uid, allocator);
    scene.AddMember("Name", rapidjson::Value(name.c_str(), allocator), allocator);
    scene.AddMember("RootGameObject", rootGameObject, allocator);

    // Serialize GameObjects
    rapidjson::Value gameObjectsJSON(rapidjson::kArrayType);

    for (const auto &[uid, gameObject] : gameObjects)
    {
        if (!gameObject) continue;

        rapidjson::Value goJSON(rapidjson::kObjectType);

        goJSON.AddMember("UID", uid, allocator);
        goJSON.AddMember("ParentUID", gameObject->GetParent(), allocator);
        goJSON.AddMember("Name", rapidjson::Value(gameObject->GetName().c_str(), allocator), allocator);

        // Child UUIDs
        rapidjson::Value childUIDs(rapidjson::kArrayType);
        for (UID childUID : gameObject->GetChildren())
        {
            childUIDs.PushBack(childUID, allocator);
        }
        goJSON.AddMember("Children", childUIDs, allocator);
        goJSON.AddMember("RootComponentUID", gameObject->GetRootComponent()->GetUID(), allocator);

        gameObjectsJSON.PushBack(goJSON, allocator);
    }

    // Add gameObjects to scene
    scene.AddMember("GameObjects", gameObjectsJSON, allocator);

    // Serialize Components
    rapidjson::Value componentsJSON(rapidjson::kArrayType);

    for (const auto &[uid, component] : components)
    {
        if (!component) continue;

        rapidjson::Value componentJSON(rapidjson::kObjectType);

        component->Save(componentJSON, allocator);

        componentsJSON.PushBack(componentJSON, allocator);
    }

    // Add components to scene
    scene.AddMember("Components", componentsJSON, allocator);

    doc.AddMember("Scene", scene, allocator);

    // Save file like JSON
    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    std::string sceneFilePath;
    std::string fileName;

    if (saveMode == SaveMode::Save)
    {
        sceneFilePath = std::string(path) + std::to_string(sceneUID) + SCENE_EXTENSION;
        fileName      = sceneName;
    }
    else
    {
        sceneFilePath = FileSystem::GetFilePath(path) + std::to_string(sceneUID) + SCENE_EXTENSION;
        fileName      = FileSystem::GetFileNameWithoutExtension(path);
    }

    unsigned int bytesWritten =
        (unsigned int)FileSystem::Save(sceneFilePath.c_str(), buffer.GetString(), buffer.GetSize(), false);
    if (bytesWritten == 0)
    {
        GLOG("Failed to save scene file: %s", path);
        return false;
    }

    GLOG("%s saved as scene", fileName.c_str());

    return true;
}

bool LibraryModule::LoadScene(const char *path)
{
    rapidjson::Document doc;
    bool loaded = FileSystem::LoadJSON(path, doc);

    if (!loaded)
    {
        GLOG("Failed to load scene file: %s", path);
        return false;
    }

    if (!doc.HasMember("Scene") || !doc["Scene"].IsObject())
    {
        GLOG("Invalid scene format: %s", path);
        return false;
    }

    rapidjson::Value &scene = doc["Scene"];

    // Scene values
    UID sceneUID            = scene["UID"].GetUint64();
    std::string name        = scene["Name"].GetString();
    UID rootGameObject      = scene["RootGameObject"].GetUint64();

    if (sceneUID == App->GetSceneModule()->GetSceneUID())
    {
        GLOG("Scene already loaded: %s", name.c_str());
        return false;
    }

    App->GetSceneModule()->CloseScene();

    // Deserialize Components
    if (scene.HasMember("Components") && scene["Components"].IsArray())
    {
        const rapidjson::Value &components = scene["Components"];

        for (rapidjson::SizeType i = 0; i < components.Size(); i++)
        {
            const rapidjson::Value &component = components[i];

            Component *newComponent           = ComponentUtils::CreateExistingComponent(component);

            UID componentUID                  = component["UID"].GetUint64();
            if (newComponent != nullptr) App->GetSceneModule()->AddComponent(componentUID, newComponent);
        }
    }

    // Deserialize GameObjects
    if (scene.HasMember("GameObjects") && scene["GameObjects"].IsArray())
    {
        const rapidjson::Value &gameObjects = scene["GameObjects"];
        for (rapidjson::SizeType i = 0; i < gameObjects.Size(); i++)
        {
            const rapidjson::Value &gameObject = gameObjects[i];

            UID goUID                          = gameObject["UID"].GetUint64();
            UID parentUID                      = gameObject["ParentUID"].GetUint64();
            std::string goName                 = gameObject["Name"].GetString();
            UID rootComponentUUID              = gameObject["RootComponentUID"].GetUint64();

            GameObject *newGameObject          = new GameObject(parentUID, goName, rootComponentUUID);
            newGameObject->SetUUID(goUID);

            if (gameObject.HasMember("Children") && gameObject["Children"].IsArray())
            {
                const rapidjson::Value &childUIDs = gameObject["Children"];
                for (rapidjson::SizeType j = 0; j < childUIDs.Size(); j++)
                {
                    UID childUID = childUIDs[j].GetUint64();
                    newGameObject->AddChildren(childUID);
                }
            }

            if (newGameObject != nullptr) App->GetSceneModule()->AddGameObject(goUID, newGameObject);
        }
    }

    App->GetSceneModule()->LoadScene(sceneUID, name.c_str(), rootGameObject);

    GLOG("%s scene loaded", name.c_str());
    return true;
}

bool LibraryModule::LoadLibraryMaps()
{

    for (const auto &entry : std::filesystem::recursive_directory_iterator(LIBRARY_PATH))
    {

        if (entry.is_regular_file())
        {
            std::string filePath = entry.path().string();

            // Generate UID using the function from globals.h
            UID originalUID      = GenerateUID();

            // Modify the UID based on file extension
            UID finalUID         = LibraryModule::AssignFiletypeUID(originalUID, filePath);

            UID prefix           = finalUID / 100000000000000;

            switch (prefix)
            {
            case 1:
                AddMesh(finalUID, filePath);
                break;
            case 2:
                AddMaterial(finalUID, filePath);
                break;
            case 3:
                AddTexture(finalUID, filePath);
                break;
            default:
                GLOG("Category: Unknown File Type (99)");
                break;
            }
        }
    }

    return true;
}

UID LibraryModule::AssignFiletypeUID(UID originalUID, const std::string &filePath)
{

    uint64_t prefix = 99; // Default prefix "99" for unknown files
    if (FileSystem::GetFileExtension(filePath) == MESH_EXTENSION)
    {
        prefix = 01;
    }
    if (FileSystem::GetFileExtension(filePath) == MATERIAL_EXTENSION)
    {
        prefix = 02;
    }
    if (FileSystem::GetFileExtension(filePath) == TEXTURE_EXTENSION)
    {
        prefix = 03;
    }
    // GLOG("%llu", prefix)
    uint64_t final = (prefix * 100000000000000) + (originalUID % 100000000000000);
    GLOG("%llu", final);
    return final;
}

void LibraryModule::AddTexture(UID textureUID, const std::string &ddsPath)
{
    textureMap[ddsPath] = textureUID; // Map the texture UID to its DDS path
}

void LibraryModule::AddMesh(UID meshUID, const std::string &sobPath)
{
    meshMap[sobPath] = meshUID; // Map the texture UID to its DDS path
}

void LibraryModule::AddMaterial(UID materialUID, const std::string &matPath)
{
    meshMap[matPath] = materialUID; // Map the texture UID to its DDS path
}

UID LibraryModule::GetTextureUID(const std::string &texturePath) const
{

    auto it = textureMap.find(texturePath);
    if (it != textureMap.end())
    {
        return it->second;
    }

    return 0;
}

UID LibraryModule::GetMeshUID(const std::string &meshPath) const
{

    auto it = meshMap.find(meshPath);
    if (it != meshMap.end())
    {
        return it->second;
    }

    return 0;
}

UID LibraryModule::GetMaterialUID(const std::string &materialPath) const
{

    auto it = materialMap.find(materialPath);
    if (it != materialMap.end())
    {
        return it->second;
    }

    return 0;
}

const std::string &LibraryModule::GetResourcePath(UID resourceID)
{
    auto it = resourcePathsMap.find(resourceID);
    if (it != resourcePathsMap.end())
    {
        return it->second;
    }

    return "";
}
