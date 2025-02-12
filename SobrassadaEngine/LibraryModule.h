#pragma once

#include "Module.h"
#include <string>
#include <unordered_map>

class LibraryModule : public Module
{

    public:
    LibraryModule();
    ~LibraryModule();
    // Add a texture to the manager (maps texture name to DDS path)
    void AddTexture(const std::string &imageName, const std::string &ddsPath);

    bool Init() override;

    bool SaveScene(const char *path);
    bool LoadScene(const char *path);
    // Get the DDS path for a texture by its name (returns an empty string if not found)
    std::string GetTextureDDSPath(const std::string &imageName) const;

    private:
    // scenes
    // models
    // materials
    // textures
    // lights

    std::unordered_map<std::string, std::string> textureMap; // Name -> DDS path

};
