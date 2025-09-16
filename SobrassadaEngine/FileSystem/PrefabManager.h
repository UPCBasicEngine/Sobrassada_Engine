#pragma once

#include "Globals.h"

#include <string>

class ResourcePrefab;
class GameObject;

namespace PrefabManager
{
    UID SavePrefab(const GameObject* gameObject, bool override);
    void CopyPrefab(
        const std::string& filePath, const std::string& targetFilePath, const std::string& name, const UID sourceUID
    );
    SOBRASADA_API_ENGINE ResourcePrefab* LoadPrefab(UID prefabUID);
} // namespace PrefabManager
