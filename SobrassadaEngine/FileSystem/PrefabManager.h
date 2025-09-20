#pragma once

#include "Globals.h"

#include <string>
#include <unordered_map>

class GameObject;

namespace PrefabManager
{
    void SavePrefab(GameObject* gameObject);
    void CopyPrefab(
        const std::string& filePath, const std::string& targetFilePath, const std::string& name, const UID sourceUID
    );
    SOBRASADA_API_ENGINE void LoadPrefab(UID prefabUID, GameObject* targetGO, std::unordered_map<UID, GameObject*>& outGameObjects);

    void UpdateBonesIfNecessary(const GameObject* target, UID staticModUID, const std::unordered_map<UID, GameObject*>& gameObjects);
} // namespace PrefabManager
