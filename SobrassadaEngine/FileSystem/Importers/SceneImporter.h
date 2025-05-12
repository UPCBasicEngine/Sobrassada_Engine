#pragma once

#include "Globals.h"

#include <rapidjson/document.h>

namespace tinygltf
{
    class Model;
} // namespace tinygltf

namespace SceneImporter
{
    void Import(const char* filePath);
    void ImportGLTF(const char* filePath, const std::string& targetFilePath);
    void CopyGLTF(const char* filePath, const std::string& targetFilePath, std::string& copiedFilePath);
    tinygltf::Model LoadModelGLTF(const char* filePath);
    void ImportMeshFromMetadata(
        const std::string& filePath, const std::string& targetFilePath, const std::string& name,
        const rapidjson::Value& importOptions, UID sourceUID
    );
    void ImportMaterialFromMetadata(
        const std::string& filePath, const std::string& targetFilePath, const std::string& name, UID sourceUID
    );
    void ImportAnimationFromMetadata(
        const std::string& filePath, const std::string& targetFilePath, const std::string& name, UID sourceUID,
        const rapidjson::Value& importOptions
    );
    void
    CopyPrefab(const std::string& filePath, const std::string& targetFilePath, const std::string& name, UID sourceUID);
    void
    CopyModel(const std::string& filePath, const std::string& targetFilePath, const std::string& name, UID sourceUID);
    void
    CopyFont(const std::string& filePath, const std::string& targetFilePath, const std::string& name, UID sourceUID);
    void CreateLibraryDirectories(const std::string& projectFilePath);

}; // namespace SceneImporter