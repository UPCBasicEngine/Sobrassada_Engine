#include "TextureModuleTest.h"

#include "DirectXTex/DirectXTex.h"
#include "glew.h"
#include <TextureImporter.h>
#include <string>

TextureModuleTest::TextureModuleTest() {}

TextureModuleTest::~TextureModuleTest() {}

unsigned int TextureModuleTest::LoadTexture(const wchar_t *texturePath, DirectX::TexMetadata &outTexMetadata)
{
    GLOG("Loading texture: %s", texturePath)
    unsigned int textureId = 0;

    DirectX::ScratchImage scratchImage;
    OpenGLMetadata openGlMeta;
    
    bool succeded          = LoadTextureFile(texturePath, outTexMetadata, scratchImage);

    if (succeded)
    {
        ConvertMetadataOld(outTexMetadata, openGlMeta);

        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);

        // Sending texture to OpgenGL
        glTexImage2D(
            GL_TEXTURE_2D, 0, openGlMeta.internalFormat, outTexMetadata.width, outTexMetadata.height, 0,
            openGlMeta.format, openGlMeta.type, scratchImage.GetPixels()
        );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    }

    return textureId;
}

unsigned int TextureModuleTest::LoadCubemap(const wchar_t *texturePath) const
{
    unsigned int textureId = 0;

    DirectX::ScratchImage scratchImage;
    DirectX::TexMetadata texMetadata;
    OpenGLMetadata openGlMeta;

    bool succeded = LoadTextureFile(texturePath, texMetadata, scratchImage);
    if (succeded)
    {
        ConvertMetadataOld(texMetadata, openGlMeta);

        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_CUBE_MAP, textureId);

        // Sending texture to OpgenGL
        for (int i = 0; i < texMetadata.arraySize; ++i)
        {
            const DirectX::Image* face = scratchImage.GetImage(0, i, 0);
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, openGlMeta.internalFormat, face->width, face->height, 0, openGlMeta.format, openGlMeta.type, face->pixels);
		}
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    }
    return textureId;
}

void TextureModuleTest::ConvertMetadataOld(const DirectX::TexMetadata &metadata, OpenGLMetadata &outMetadata)
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

unsigned int TextureModuleTest::CreateFramebufferTexture(int width, int height)
{
    unsigned int textureId = 0;

    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    return textureId;
}

bool TextureModuleTest::LoadTextureFile(
    const wchar_t *texturePath, DirectX::TexMetadata &outMetadata, DirectX::ScratchImage &outImage
) const
{
    HRESULT hr = LoadFromDDSFile(texturePath, DirectX::DDS_FLAGS_NONE, &outMetadata, outImage);

    if (SUCCEEDED(hr)) return true;

    hr = LoadFromTGAFile(texturePath, DirectX::TGA_FLAGS_NONE, &outMetadata, outImage);

    if (SUCCEEDED(hr)) return true;

    hr = DirectX::LoadFromWICFile(texturePath, DirectX::WIC_FLAGS_NONE, &outMetadata, outImage);

    if (SUCCEEDED(hr)) return true;

    return false;
}
