#pragma once

#include "Resource.h"

#include "DirectXTex/DirectXTex.h"

struct OpenGLMetadata
{
    unsigned internalFormat;
    unsigned format;
    unsigned type;
};

class ResourceTexture : public Resource
{
  public:
    ResourceTexture(UID uid, const std::string& name);
    ~ResourceTexture() override;

    void LoadData(const DirectX::TexMetadata& metadata, const DirectX::ScratchImage& scratchImage);
    static void ConvertMetadata(const DirectX::TexMetadata& metadata, OpenGLMetadata& outMetadata);

    unsigned int GetTextureID() const { return textureID; }
    int GetTextureWidth() const { return (int)metadata.width; }
    int GetTextureHeight() const { return (int)metadata.height; }
    int GetMipMapLevels() const { return static_cast<int>(metadata.mipLevels); }

    void SetTextureID(unsigned int id) { textureID = id; }
    bool IsCubemap() const { return (metadata.miscFlags & DirectX::TEX_MISC_FLAG::TEX_MISC_TEXTURECUBE) != 0; }
    unsigned int GetCubemapFaceID(int faceIndex) const;


  private:
    unsigned int textureID = 0;
    DirectX::ScratchImage scratchImage; // TODO: needed?
    DirectX::TexMetadata metadata;
    OpenGLMetadata openGLMetadata;
    unsigned int m_CubemapFaceIDs[6] = {0};
};
