#pragma once

namespace SceneImporter
{
    void Import(const char *filePath);
    void ImportGLTF(const char *filePath);
    void CreateLibraryDirectories();
}; // namespace SceneImporter