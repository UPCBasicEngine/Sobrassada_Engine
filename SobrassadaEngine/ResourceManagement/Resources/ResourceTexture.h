#pragma once
#include "Resource.h"
#include "TextureModuleTest.h"
#include "DirectXTex/DirectXTex.h"

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
