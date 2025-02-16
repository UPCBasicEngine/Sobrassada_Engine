#pragma once

#include "Globals.h"
#include "ResourceManagement/Resources/ResourceTexture.h"

class ResourceTexture;

namespace TextureImporter
{
    UID Import(const char* filePath);
    ResourceTexture* LoadTexture(UID textureUID);
    unsigned int LoadCubemap(const char* texturePath);
    bool LoadTextureFile(const wchar_t* texturePath, DirectX::TexMetadata& outMetadata, DirectX::ScratchImage& outImage);

}; // namespace TextureImporter
