#pragma once

#include "Globals.h"

class ResourceTexture;

namespace TextureImporter
{
    UID Import(const char* filePath);
    ResourceTexture* LoadTexture(UID textureUID);
}; // namespace TextureImporter
