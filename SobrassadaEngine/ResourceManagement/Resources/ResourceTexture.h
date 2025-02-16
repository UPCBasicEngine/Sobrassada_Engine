#pragma once

#include "Resource.h"

#include "DirectXTex/DirectXTex.h"

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

class ResourceTexture : public Resource
{
public:
    ResourceTexture(UID uid, const std::string & name);
    ~ResourceTexture() override;

    void LoadData(const DirectX::TexMetadata& metadata, const DirectX::ScratchImage& scratchImage);
    static void ConvertMetadata(const DirectX::TexMetadata &metadata, OpenGLMetadata &outMetadata);

private:
    DirectX::TexMetadata metadata;
    OpenGLMetadata openGLMetadata;
    DirectX::ScratchImage scratchImage;
};
