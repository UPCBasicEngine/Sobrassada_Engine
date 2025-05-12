#pragma once

#include "Globals.h"

#include <string>

class ResourceTexture;

namespace DirectX
{
    class ScratchImage;
    struct TexMetadata;
} // namespace DirectX

namespace TextureImporter
{
    UID Import(const char* filePath, const std::string& targetFilePath, UID sourceUID = INVALID_UID, bool overwriteMeta = false);
    ResourceTexture* LoadTexture(UID textureUID);
    ResourceTexture* LoadCubemap(UID textureUID);
    bool
    LoadTextureFile(const wchar_t* texturePath, DirectX::TexMetadata& outMetadata, DirectX::ScratchImage& outImage);
}; // namespace TextureImporter
