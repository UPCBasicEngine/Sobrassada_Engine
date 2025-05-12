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
#include <queue>

namespace PrefabManager
{
    UID SavePrefab(const GameObject* gameObject, bool override)
    {
        // Create doc JSON
        rapidjson::Document doc;
        doc.SetObject();
        rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

        rapidjson::Value prefab(rapidjson::kObjectType);

        // Scene values
        const UID uid = GenerateUID();
        const UID finalPrefabUID =
            override ? gameObject->GetPrefabUID() : App->GetLibraryModule()->AssignFiletypeUID(uid, FileType::Prefab);
        const std::string& savePath = App->GetProjectModule()->GetLoadedProjectPath() + PREFABS_LIB_PATH +
                                      std::to_string(finalPrefabUID) + PREFAB_EXTENSION;

        std::string name = gameObject->GetName();

        // Create structure
        prefab.AddMember("UID", finalPrefabUID, allocator);
        prefab.AddMember("Name", rapidjson::Value(name.c_str(), allocator), allocator);

        // Serialize GameObjects
        rapidjson::Value gameObjectsJSON(rapidjson::kArrayType);
        std::queue<const GameObject*> queue; // Traverse all gameObjects using a queue to avoid recursiveness
        queue.push(gameObject);

        while (queue.size() > 0)
        {
            const GameObject* currentGameObject = queue.front();

            rapidjson::Value goJSON(rapidjson::kObjectType);
            currentGameObject->Save(goJSON, allocator);
            gameObjectsJSON.PushBack(goJSON, allocator);

            for (UID child : currentGameObject->GetChildren())
            {
                queue.push(App->GetSceneModule()->GetScene()->GetGameObjectByUID(child));
            }

            queue.pop();
        }

        // Add gameObjects to scene
        prefab.AddMember("GameObjects", gameObjectsJSON, allocator);

        doc.AddMember("Prefab", prefab, allocator);

        // Save file like JSON
        rapidjson::StringBuffer buffer;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
        doc.Accept(writer);

        // Create meta file
        std::string assetPath = PREFABS_ASSETS_PATH + name + PREFAB_EXTENSION;
        MetaPrefab meta(finalPrefabUID, assetPath);
        meta.Save(name, assetPath);

        assetPath = App->GetProjectModule()->GetLoadedProjectPath() + PREFABS_ASSETS_PATH + name + PREFAB_EXTENSION;
        // Save in assets
        unsigned int bytesWritten = (unsigned int
        )FileSystem::Save(assetPath.c_str(), buffer.GetString(), (unsigned int)buffer.GetSize(), false);
        if (bytesWritten == 0)
        {
            GLOG("Failed to save prefab file: %s", assetPath);
            return INVALID_UID;
        }

        // Save in library
        bytesWritten =
            (unsigned int)FileSystem::Save(savePath.c_str(), buffer.GetString(), (unsigned int)buffer.GetSize(), false);
        if (bytesWritten == 0)
        {
            GLOG("Failed to save prefab file: %s", savePath);
            return INVALID_UID;
        }

        // Add the prefab to the resources map
        App->GetLibraryModule()->AddPrefab(finalPrefabUID, name);
        App->GetLibraryModule()->AddName(name, finalPrefabUID);
        App->GetLibraryModule()->AddResource(savePath, finalPrefabUID);

        return finalPrefabUID;
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

        UID uid                  = prefab["UID"].GetUint64();
        std::string name         = prefab["Name"].GetString();

        std::vector<GameObject*> loadedGameObjects;
        std::vector<int> parentIndices;
        if (prefab.HasMember("GameObjects") && prefab["GameObjects"].IsArray())
        {
            const rapidjson::Value& gameObjects = prefab["GameObjects"];
            for (rapidjson::SizeType i = 0; i < gameObjects.Size(); i++)
            {
                const rapidjson::Value& gameObject = gameObjects[i];
                GameObject* newObject              = new GameObject(gameObject);
                newObject->LoadData(gameObject);

                int index = 0;
                for (const GameObject* obj : loadedGameObjects)
                {
                    if (obj->GetUID() == newObject->GetParent()) break;
                    ++index;
                }
                parentIndices.push_back(index
                ); // We will ignore the parent index of the root object, so it's fine this is also 0 for it
                loadedGameObjects.push_back(newObject);
            }
        }
        ResourcePrefab* resourcePrefab = new ResourcePrefab(uid, name);
        resourcePrefab->LoadData(loadedGameObjects, parentIndices);
        return resourcePrefab;
    }
} // namespace PrefabManager
