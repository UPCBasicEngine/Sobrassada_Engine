#pragma once

#include <string>

namespace SceneImporter
{
    void Import(const char *filePath);
    void ImportGLTF(const char *filePath);
    void CreateLibraryDirectories();
    uint64_t GenerateUUID();
}; // namespace SceneImporter