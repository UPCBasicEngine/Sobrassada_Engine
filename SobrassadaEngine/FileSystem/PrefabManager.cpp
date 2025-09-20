#include "PrefabManager.h"

#include "Application.h"
#include "FileSystem.h"
#include "GameObject.h"
#include "LibraryModule.h"
#include "MetaPrefab.h"
#include "ProjectModule.h"
#include "SceneModule.h"
#include "Scripting/SobrassadaScripts/SobrassadaScripts/GameSession.h"
#include "Standalone/AnimationComponent.h"
#include "Standalone/MeshComponent.h"

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

        prefab.AddMember("UID", finalPrefabUID, allocator);

        rapidjson::Value root(rapidjson::kObjectType);

        gameObject->SetPrefabUID(finalPrefabUID);
        gameObject->Save(root, allocator);

        prefab.AddMember("Root", root, allocator);

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

    void LoadPrefab(UID prefabUID, GameObject* targetGO, std::unordered_map<UID, GameObject*>& outGameObjects)
    {
        targetGO->SetPrefabUID(prefabUID);
        outGameObjects.erase(targetGO->GetUID());
        
        std::stack<GameObject*> prefabBuffer;   // Prefab uids for iterative iteration
        prefabBuffer.push(targetGO);

        const UID baseTargetUID = targetGO->GetUID();
        const bool baseTargetEnabled = targetGO->IsEnabled();

        std::unordered_map<HashString, std::vector<std::pair<GameObject*, int>>> gameObjectsToInit;
        std::vector<GameObject*> goBoneUpdate;

        // All uids of the prefab will be summed with this to generate new unique ids for the scene game object tree
        const UID staticModUID = GenerateUID();

        while (!prefabBuffer.empty())
        {
            GameObject* currentTargetGO = prefabBuffer.top();
            prefabBuffer.pop();
            
            rapidjson::Document doc;
            const std::string& filepath = App->GetLibraryModule()->GetResourcePath(currentTargetGO->GetPrefabUID());

            std::vector<std::pair<GameObject*, int>> cachedGameObjects;

            bool loaded          = FileSystem::LoadJSON(filepath.c_str(), doc);
            if (!loaded)
            {
                GLOG("Failed to load prefab file: %s", filepath.c_str());
                return;
            }
            if (!doc.HasMember("Prefab") || !doc["Prefab"].IsObject())
            {
                GLOG("Invalid prefab format: %s", filepath.c_str());
                return;
            }
            
            rapidjson::Value& prefab = doc["Prefab"];
            rapidjson::Value& root = prefab["Root"];
        
            GameObject* rootGO = new GameObject(root);
            rootGO->ModifyAllUIDsBy(staticModUID);
            rootGO->SetUID(currentTargetGO->GetUID());
            rootGO->SetParent(currentTargetGO->GetParent());
            rootGO->SetLocalTransform(currentTargetGO->GetLocalTransform(), false);
            
            outGameObjects.insert({rootGO->GetUID(), rootGO});
            cachedGameObjects.emplace_back(std::pair(rootGO, -1));
        
            if (prefab.HasMember("GameObjects") && prefab["GameObjects"].IsArray())
            {
                const rapidjson::Value& gameObjects = prefab["GameObjects"];
                
                for (rapidjson::SizeType i = 0; i < gameObjects.Size(); i++)
                {
                    const rapidjson::Value& gameObject = gameObjects[i];
                    GameObject* newObject              = new GameObject(gameObject);
                    if (newObject->GetPrefabUID() != INVALID_UID)
                    {
                        prefabBuffer.push(newObject);
                        continue;
                    }
                    newObject->ModifyAllUIDsBy(staticModUID);
                    outGameObjects.insert({newObject->GetUID(), newObject});
                    cachedGameObjects.emplace_back(newObject, i);
                }
                
                for (UID childUID : rootGO->GetChildren())
                {
                    if (outGameObjects.find(childUID) != outGameObjects.end())
                        outGameObjects.at(childUID)->SetParent(rootGO->GetUID());
                }
            }

            gameObjectsToInit.insert({filepath, cachedGameObjects});
            
            delete currentTargetGO; // This game object is replaced by the root of the prefab from the prefab file
            currentTargetGO = nullptr;
        }

        for (const auto& pair : gameObjectsToInit)
        {
            rapidjson::Document doc;
            const std::string& filepath = App->GetLibraryModule()->GetResourcePath(pair.second[0].first->GetPrefabUID());

            bool loaded          = FileSystem::LoadJSON(filepath.c_str(), doc);
            if (!loaded)
            {
                GLOG("Failed to load prefab file: %s", filepath.c_str());
                return;
            }
            if (!doc.HasMember("Prefab") || !doc["Prefab"].IsObject())
            {
                GLOG("Invalid prefab format: %s", filepath.c_str());
                return;
            }
            
            rapidjson::Value& prefab = doc["Prefab"];
            rapidjson::Value& root = prefab["Root"];

            if (prefab.HasMember("GameObjects") && prefab["GameObjects"].IsArray())
            {
                const rapidjson::Value& gameObjects = prefab["GameObjects"];

                for (auto goIndexPair : pair.second)
                {
                    if (goIndexPair.second == -1)
                        goIndexPair.first->LoadData(root);
                    else
                        goIndexPair.first->LoadData(gameObjects[goIndexPair.second]);

                    const MeshComponent* mesh = goIndexPair.first->GetComponent<MeshComponent*>();
                    if (mesh != nullptr && !mesh->GetBones().empty()) goBoneUpdate.emplace_back(goIndexPair.first);
                }
            }
        }
        gameObjectsToInit.clear();

        for (const GameObject* go : goBoneUpdate)
            UpdateBonesIfNecessary(go, staticModUID, outGameObjects);
        
        App->GetSceneModule()->GetScene()->GetGameObjectByUID(baseTargetUID)->SetEnabledRecursive(baseTargetEnabled);
    }

    void UpdateBonesIfNecessary(const GameObject* target, UID staticModUID, const std::unordered_map<UID, GameObject*>& gameObjects)
    {
        MeshComponent* mesh = target->GetComponent<MeshComponent*>();
        if (mesh != nullptr && !mesh->GetBones().empty())
        {
            // Remap the bones references
            const std::vector<UID>& bones = mesh->GetBones();
            std::vector<UID> newBonesUIDs;
            std::vector<GameObject*> newBonesObjects;

            for (const UID bone : bones)
            {
                newBonesUIDs.push_back(bone + staticModUID);
                newBonesObjects.push_back(gameObjects.at(bone + staticModUID));
            }

            mesh->SetBones(newBonesObjects, newBonesUIDs);
        }

        // If has animations, map them here
        if (AnimationComponent* animComp = target->GetComponent<AnimationComponent*>()) animComp->SetBoneMapping();
    }
} // namespace PrefabManager
