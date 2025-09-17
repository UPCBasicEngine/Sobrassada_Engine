#include "PrefabManager.h"

#include "Application.h"
#include "FileSystem.h"
#include "GameObject.h"
#include "LibraryModule.h"
#include "MetaPrefab.h"
#include "ProjectModule.h"
#include "ResourcePrefab.h"
#include "SceneModule.h"

#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include <filesystem>
#include <stack>

namespace PrefabManager
{
    void SavePrefab(GameObject* gameObject)
    {
        // Create doc JSON
        rapidjson::Document doc;
        doc.SetObject();
        rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

        rapidjson::Value prefab(rapidjson::kObjectType);

        // Scene values
        const UID finalPrefabUID = gameObject->GetPrefabUID() != INVALID_UID ? gameObject->GetPrefabUID() :
            App->GetLibraryModule()->AssignFiletypeUID(GenerateUID(), FileType::Prefab);
            
        const std::string& savePath = App->GetProjectModule()->GetLoadedProjectPath() + PREFABS_LIB_PATH +
                                      std::to_string(finalPrefabUID) + PREFAB_EXTENSION;

        gameObject->SetPrefabUID(finalPrefabUID);
        gameObject->Save(prefab, allocator);

        // Serialize GameObjects
        rapidjson::Value gameObjectsJSON(rapidjson::kArrayType);
        
        std::stack<UID> childrenBuffer;
        for (UID child: gameObject->GetChildren())
            childrenBuffer.push(child);

        while (!childrenBuffer.empty())
        {
            GameObject* go = App->GetSceneModule()->GetScene()->GetGameObjectByUID(childrenBuffer.top());
            childrenBuffer.pop();
            if (go != nullptr)
            {
                rapidjson::Value goJSON(rapidjson::kObjectType);

                go->Save(goJSON, allocator);

                gameObjectsJSON.PushBack(goJSON, allocator);

                if (go->GetPrefabUID() == INVALID_UID)
                {
                    for (UID child : go->GetChildren())
                        childrenBuffer.push(child);
                }
            }
        }
        
        // Add gameObjects to scene
        prefab.AddMember("GameObjects", gameObjectsJSON, allocator);

        doc.AddMember("Prefab", prefab, allocator);

        // Save file like JSON
        rapidjson::StringBuffer buffer;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
        doc.Accept(writer);

        // Create meta file
        std::string assetPath = PREFABS_ASSETS_PATH + gameObject->GetName() + PREFAB_EXTENSION;
        MetaPrefab meta(finalPrefabUID, assetPath);
        meta.Save(gameObject->GetName(), assetPath);

        assetPath = App->GetProjectModule()->GetLoadedProjectPath() + PREFABS_ASSETS_PATH + gameObject->GetName() + PREFAB_EXTENSION;
        // Save in assets
        unsigned int bytesWritten = (unsigned int
        )FileSystem::Save(assetPath.c_str(), buffer.GetString(), (unsigned int)buffer.GetSize(), false);
        if (bytesWritten == 0)
        {
            GLOG("Failed to save prefab file: %s", assetPath);
            return;
        }

        // Save in library
        bytesWritten =
            (unsigned int)FileSystem::Save(savePath.c_str(), buffer.GetString(), (unsigned int)buffer.GetSize(), false);
        if (bytesWritten == 0)
        {
            GLOG("Failed to save prefab file: %s", savePath);
            return;
        }

        // Add the prefab to the resources map
        App->GetLibraryModule()->AddPrefab(finalPrefabUID, gameObject->GetName());
        App->GetLibraryModule()->AddName(gameObject->GetName(), finalPrefabUID);
        App->GetLibraryModule()->AddResource(savePath, finalPrefabUID);
    }

    void CopyPrefab(
        const std::string& filePath, const std::string& targetFilePath, const std::string& name, const UID sourceUID
    )
    {
        std::string destination = targetFilePath + PREFABS_LIB_PATH + std::to_string(sourceUID) + PREFAB_EXTENSION;
        FileSystem::Copy(filePath.c_str(), destination.c_str());

        App->GetLibraryModule()->AddPrefab(sourceUID, name);
        App->GetLibraryModule()->AddName(name, sourceUID);
        App->GetLibraryModule()->AddResource(destination, sourceUID);
    }

    ResourcePrefab* LoadPrefab(UID prefabUID)
    {
        rapidjson::Document doc;
        std::string filepath = App->GetLibraryModule()->GetResourcePath(prefabUID);

        bool loaded          = FileSystem::LoadJSON(filepath.c_str(), doc);
        if (!loaded)
        {
            GLOG("Failed to load prefab file: %s", filepath.c_str());
            return nullptr;
        }
        if (!doc.HasMember("Prefab") || !doc["Prefab"].IsObject())
        {
            GLOG("Invalid prefab format: %s", filepath.c_str());
            return nullptr;
        }

        rapidjson::Value& prefab = doc["Prefab"];

        GameObject* rootGO = new GameObject(prefab);
        rootGO->LoadData(prefab);
        
        std::vector<GameObject*> loadedGameObjects;
        loadedGameObjects.push_back(rootGO);
        
        if (prefab.HasMember("GameObjects") && prefab["GameObjects"].IsArray())
        {
            const rapidjson::Value& gameObjects = prefab["GameObjects"];
            for (rapidjson::SizeType i = 0; i < gameObjects.Size(); i++)
            {
                const rapidjson::Value& gameObject = gameObjects[i];
                GameObject* newObject              = new GameObject(gameObject);
                newObject->LoadData(gameObject);
                
                loadedGameObjects.push_back(newObject);
            }
        }
        ResourcePrefab* resourcePrefab = new ResourcePrefab(rootGO->GetUID(), rootGO->GetName());
        resourcePrefab->LoadData(rootGO->GetUID(), loadedGameObjects);
        return resourcePrefab;
    }
} // namespace PrefabManager
