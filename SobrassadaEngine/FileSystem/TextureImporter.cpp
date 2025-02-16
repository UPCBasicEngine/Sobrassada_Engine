#include "TextureImporter.h"

#include "Application.h"
#include "DirectXTex/DirectXTex.h"
#include "ResourceManagement/Resources/ResourceTexture.h"
#include "FileSystem.h"
#include "LibraryModule.h"
#include "glew.h"
#include <string>

namespace TextureImporter
{
    UID Import(const char* filePath)
    {
        // Copy image to Assets folder
        {
            std::string copyPath = ASSETS_PATH + FileSystem::GetFileNameWithExtension(filePath);
            if (!FileSystem::Exists(copyPath.c_str()))
            {
                FileSystem::Copy(filePath, copyPath.c_str());
            }
        }

        std::string textureStr = std::string(filePath);
        std::wstring wPath     = std::wstring(textureStr.begin(), textureStr.end());

        DirectX::ScratchImage image;
        HRESULT hr = DirectX::LoadFromWICFile(wPath.c_str(), DirectX::WIC_FLAGS_NONE, nullptr, image);

        if (FAILED(hr))
        {
            hr = DirectX::LoadFromTGAFile(wPath.c_str(), DirectX::TGA_FLAGS_NONE, nullptr, image);
            if (FAILED(hr))
            {
                hr = DirectX::LoadFromDDSFile(wPath.c_str(), DirectX::DDS_FLAGS_NONE, nullptr, image);
                if (FAILED(hr))
                {
                    GLOG("Failed to load texture: %s", filePath);
                    return 0;
                }
            }
        }

        DirectX::Blob blob;
        hr = DirectX::SaveToDDSMemory(
            image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::DDS_FLAGS_NONE, blob
        );

        if (FAILED(hr))
        {
            GLOG("Failed to save texture in memory: %s", filePath);
            return 0;
        }

        UID textureUID       = GenerateUID();
        std::string fileName = FileSystem::GetFileNameWithoutExtension(filePath);
        std::string savePath = TEXTURES_PATH + std::to_string(textureUID) + TEXTURE_EXTENSION;

        unsigned int size =
            FileSystem::Save(savePath.c_str(), blob.GetBufferPointer(), (unsigned int)blob.GetBufferSize());

        if (size == 0)
        {
            GLOG("Failed to save DDS file: %s", savePath.c_str());
            return 0;
        }
        std::string libraryPath = FileSystem::GetFileNameWithExtension(savePath);

        App->GetLibraryModule()->AddTexture(textureUID, libraryPath);

        GLOG("%s saved as dds", fileName.c_str());

        return textureUID;
    }

    ResourceTexture* LoadTexture(UID textureUID)
    {
        std::string path   = App->GetLibraryModule()->GetResourcePath(textureUID);

        std::wstring wPath = std::wstring(path.begin(), path.end());

        DirectX::ScratchImage outImage;
        DirectX::TexMetadata outMetadata;

        HRESULT hr = LoadFromDDSFile(wPath.c_str(), DirectX::DDS_FLAGS_NONE, &outMetadata, outImage);

        if (FAILED(hr))
        {
            hr = LoadFromTGAFile(wPath.c_str(), DirectX::TGA_FLAGS_NONE, &outMetadata, outImage);
        }

        if (FAILED(hr))
        {
            hr = DirectX::LoadFromWICFile(wPath.c_str(), DirectX::WIC_FLAGS_NONE, &outMetadata, outImage);
        }

        if (FAILED(hr))
        {
            GLOG("Failed to load texture: %s", path.c_str());
            return nullptr;
        }

        ResourceTexture* texture = new ResourceTexture(textureUID, FileSystem::GetFileNameWithoutExtension(path));

        texture->LoadData(outMetadata, outImage);

        return texture;
    }
    /*
    unsigned int LoadCubemap(const wchar_t* texturePath)
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
                glTexImage2D(
                    GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, openGlMeta.internalFormat, static_cast<GLsizei>(face->width),
                    static_cast<GLsizei>(face->height), 0, openGlMeta.format, openGlMeta.type, face->pixels
                );
            }
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        }
        return textureId;
    }
    */
}; // namespace TextureImporter
