#pragma once

#include "Globals.h"
#include "Module.h"
#include <string>
#include <unordered_map>

enum class SaveMode
{
    Save,
    SaveAs
};

class LibraryModule : public Module
{

  public:
    LibraryModule();
    ~LibraryModule();

    bool Init() override;

    bool SaveScene(const char *path, SaveMode saveMode) const;
    bool LoadScene(const char *path, bool reload = false); // const?

    bool LoadLibraryMaps();
    UID AssignFiletypeUID(UID originalUID, const std::string &filePath);

    void AddTexture(UID textureUID, const std::string &ddsPath);
    void AddMesh(UID meshUID, const std::string &matPath);
    void AddMaterial(UID materialUID, const std::string &sobPath);

    UID GetTextureUID(const std::string &texturePath) const;
    UID GetMeshUID(const std::string &meshPath) const;
    UID GetMaterialUID(const std::string &materialPath) const;

    const std::string &GetResourcePath(UID resourceID) const;

    const std::unordered_map<std::string, UID> &GetTextureMap() const { return textureMap; }
    const std::unordered_map<std::string, UID> &GetMaterialMap() const { return materialMap; }
    const std::unordered_map<std::string, UID> &GetMeshMap() const { return meshMap; }

    void AddResource(const std::string& resourcePath, UID resourceUID);

  private:
    // maps for user visual
    std::unordered_map<std::string, UID> textureMap;  // UID -> name.dds
    std::unordered_map<std::string, UID> materialMap; // UID-> name.mat
    std::unordered_map<std::string, UID> meshMap;     // UID-> name.sob

    // filled on load and import
    std::unordered_map<UID, std::string> resourcePathsMap; // UID -> all resources path
};
