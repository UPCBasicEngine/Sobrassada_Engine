#include "ResourceTexture.h"

#include <glew.h>
 ResourceTexture::ResourceTexture(uint64_t uid, const std::string& name) : Resource(uid, name, ResourceType::Texture){
}

void ResourceTexture::LoadData(const DirectX::TexMetadata& metadata, const DirectX::ScratchImage& scratchImage)
{
    this->metadata = metadata;
    this->scratchImage = scratchImage;
    ConvertMetadata(this->metadata, openGLMetadata);
}

void TextureModuleTest::ConvertMetadata(const DirectX::TexMetadata &metadata, OpenGLMetadata &outMetadata)
{
    switch (metadata.format)
    {
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
    case DXGI_FORMAT_R8G8B8A8_UNORM:
        outMetadata.internalFormat = GL_RGBA8;
        outMetadata.format         = GL_RGBA;
        outMetadata.type           = GL_UNSIGNED_BYTE;
        break;
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
    case DXGI_FORMAT_B8G8R8A8_UNORM:
        outMetadata.internalFormat = GL_RGBA8;
        outMetadata.format         = GL_BGRA;
        outMetadata.type           = GL_UNSIGNED_BYTE;
        break;
    case DXGI_FORMAT_B5G6R5_UNORM:
        outMetadata.internalFormat = GL_RGB8;
        outMetadata.format         = GL_BGR;
        outMetadata.type           = GL_UNSIGNED_BYTE;
        break;
    default:
        assert(false && "Unsupported format");
    }
}