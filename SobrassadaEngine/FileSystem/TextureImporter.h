#pragma once

#include "Globals.h"
#include "ResourceManagement/Resources/ResourceTexture.h"

class ResourceTexture;

namespace TextureImporter
{
    UID Import(const char* filePath);
    ResourceTexture* LoadTexture(UID textureUID);
}; // namespace TextureImporter
