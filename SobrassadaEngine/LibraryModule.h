#pragma once

#include "Module.h"

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

    void AddTexture(const std::string &imageName, const std::string &ddsPath);
    std::string GetTextureDDSPath(const std::string &imageName) const;

  private:
    std::unordered_map<std::string, std::string> textureMap; // Name -> DDS path
    // librarymeshes & librarytextures map
};
