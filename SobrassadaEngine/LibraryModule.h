#pragma once

#include "Module.h"

class LibraryModule : public Module
{
  public:
    LibraryModule();
    ~LibraryModule();

    bool Init() override;

    bool SaveScene(const char *path);
    bool LoadScene(const char *path);

  private:
    // scenes
    // models
    // materials
    // textures
    // lights
};
