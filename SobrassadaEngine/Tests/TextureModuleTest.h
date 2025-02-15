#pragma once

#include "Module.h"

struct OpenGLMetadata
{
    unsigned internalFormat;
    unsigned format;
    unsigned type;
};

namespace DirectX
{
    struct TexMetadata;
    class ScratchImage;
} // namespace DirectX

class TextureModuleTest : public Module
{
  public:
    TextureModuleTest();
    ~TextureModuleTest();

    unsigned int LoadTexture(const wchar_t *texturePath, DirectX::TexMetadata &outTexMetadata);
    unsigned int LoadCubemap(const wchar_t *texturePath) const;
    static void ConvertMetadataOld(const DirectX::TexMetadata &metadata, OpenGLMetadata &outMetadata);

    unsigned int CreateFramebufferTexture(int width, int height);

  private:
    bool
    LoadTextureFile(const wchar_t *texturePath, DirectX::TexMetadata &outMetadata, DirectX::ScratchImage &outImage) const;
};
