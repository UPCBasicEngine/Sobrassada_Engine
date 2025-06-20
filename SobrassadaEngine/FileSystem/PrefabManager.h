#pragma once

#include "Globals.h"

#include <string>

class ResourcePrefab;
class GameObject;

namespace PrefabManager
{
    UID SavePrefab(const GameObject* gameObject, bool override, const UID versionUID);
    void CopyPrefab(
        const std::string& filePath, const std::string& targetFilePath, const std::string& name, const UID sourceUID
    );
    ResourcePrefab* LoadPrefab(UID prefabUID);
} // namespace PrefabManager
