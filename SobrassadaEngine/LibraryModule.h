#pragma once

#include "Module.h"
#include "Globals.h"
#include <string>
#include <unordered_map>

class LibraryModule : public Module
{

    public:
    LibraryModule();
    ~LibraryModule();

    bool Init() override;

    bool SaveScene(const char *path) const;
    bool LoadScene(const char *path);

    void AddTexture(UID textureUID, const std::string &ddsPath);
    void AddMesh(UID meshUID, const std::string &matPath);
    void AddMaterial(UID materialUID, const std::string &sobPath);

    std::string GetTextureName(UID textureUID) const;
    std::string GetMeshName(UID textureUID) const;
    std::string GetMaterialName(UID textureUID) const;

  private:
    std::unordered_map<UID, std::string> textureMap;  // UID -> name.dds
    std::unordered_map<UID, std::string> materialMap; // UID-> name.mat
    std::unordered_map<UID, std::string> meshMap;     // UID-> name.sob
};
