#include "TextureImporter.h"

#include "Application.h"
#include "FileSystem.h"
#include "LibraryModule.h"
#include "MetaTexture.h"
#include "ProjectModule.h"
#include "ResourceTexture.h"

#include "DirectXTex/DirectXTex.h"
#include "glew.h"

namespace TextureImporter
{
    UID Import(const char* sourceFilePath, const std::string& targetFilePath, UID sourceUID)
    {
        // Copy image to Assets folder
        const std::string relativePath = ASSETS_PATH + FileSystem::GetFileNameWithExtension(sourceFilePath);
        std::string copyPath           = targetFilePath + relativePath;
        if (!FileSystem::Exists(copyPath.c_str()))
        {
            FileSystem::Copy(sourceFilePath, copyPath.c_str());
        }

        std::string textureStr = std::string(sourceFilePath);
        std::wstring wPath     = std::wstring(textureStr.begin(), textureStr.end());

        DirectX::ScratchImage image;
        HRESULT hr = DirectX::LoadFromWICFile(wPath.c_str(), DirectX::WIC_FLAGS_NONE, nullptr, image);

        bool isTextureHDR = false;

        if (FAILED(hr))
        {
            hr = DirectX::LoadFromTGAFile(wPath.c_str(), DirectX::TGA_FLAGS_NONE, nullptr, image);
            if (FAILED(hr))
            {
                hr = DirectX::LoadFromDDSFile(wPath.c_str(), DirectX::DDS_FLAGS_NONE, nullptr, image);
                if (FAILED(hr))
                {
                    hr = DirectX::LoadFromHDRFile(wPath.c_str(), nullptr, image);
                    isTextureHDR = true;
                    if (FAILED(hr))
                    {
                        GLOG("Failed to load texture: %s", sourceFilePath);
                        return 0;
                    }
                }
            }
        }

        DirectX::Blob blob;
        hr = DirectX::SaveToDDSMemory(
            image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::DDS_FLAGS_NONE, blob
        );

        if (FAILED(hr))
        {
            GLOG("Failed to save texture in memory: %s", sourceFilePath);
            return 0;
        }

        std::string fileName = FileSystem::GetFileNameWithoutExtension(sourceFilePath);
        if (isTextureHDR)
        {
            fileName += "_HDR";
        }

        UID finalTextureUID;
        if (sourceUID == INVALID_UID)
        {
            UID textureUID  = GenerateUID();
            textureUID = App->GetLibraryModule()->AssignFiletypeUID(textureUID, FileType::Texture);

            UID prefix                 = textureUID / UID_PREFIX_DIVISOR;
            const std::string savePath = App->GetProjectModule()->GetLoadedProjectPath() + METADATA_PATH +
                                         std::to_string(prefix) + FILENAME_SEPARATOR + fileName + META_EXTENSION;
            finalTextureUID = App->GetLibraryModule()->GetUIDFromMetaFile(savePath);
            if (finalTextureUID == INVALID_UID) finalTextureUID = textureUID;
            
            MetaTexture meta(finalTextureUID, relativePath, (int)image.GetMetadata().mipLevels);
            meta.Save(fileName, relativePath);
        }
        else finalTextureUID = sourceUID;

        std::string saveFilePath = targetFilePath + TEXTURES_PATH + std::to_string(finalTextureUID) + TEXTURE_EXTENSION;
        unsigned int bytesWritten =
            FileSystem::Save(saveFilePath.c_str(), blob.GetBufferPointer(), (unsigned int)blob.GetBufferSize());

        if (bytesWritten == 0)
        {
            GLOG("Failed to save DDS file: %s", fileName.c_str());
            return 0;
        }

        // added texture to textures map
        App->GetLibraryModule()->AddTexture(finalTextureUID, fileName);
        App->GetLibraryModule()->AddName(fileName, finalTextureUID);
        App->GetLibraryModule()->AddResource(saveFilePath, finalTextureUID);

        //GLOG("%s saved as dds", fileName.c_str());

        return finalTextureUID;
    }

    ResourceTexture* LoadTexture(UID textureUID)
    {
        const std::string& path     = App->GetLibraryModule()->GetResourcePath(textureUID);

        const std::string& filename = App->GetLibraryModule()->GetResourceName(textureUID);

        if (path.empty()) return nullptr; // TODO Use fallback texture instead

        const std::wstring& wPath = std::wstring(path.begin(), path.end());

        DirectX::ScratchImage scratchImage;
        DirectX::TexMetadata texMetadata;
        OpenGLMetadata openGlMeta;

        const bool succeded = LoadTextureFile(wPath.c_str(), texMetadata, scratchImage);
        if (!succeded)
        {
            GLOG("Failed to load texture: %s", path.c_str());
            return nullptr;
        }

        ResourceTexture* texture = new ResourceTexture(textureUID, filename);

        texture->LoadData(texMetadata, scratchImage);
        unsigned int textureID;
        glGenTextures(1, &textureID);

        glBindTexture(GL_TEXTURE_2D, textureID);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, static_cast<GLint>(texMetadata.mipLevels - 1));

        ResourceTexture::ConvertMetadata(texMetadata, openGlMeta);

        const int maxMipmapLevel = static_cast<int>(texMetadata.mipLevels - 1);
        if (texMetadata.mipLevels > 1)
        {
            for (size_t i = 0; i < texMetadata.mipLevels; ++i)
            {
                const DirectX::Image* mip = scratchImage.GetImage(i, 0, 0);
                glTexImage2D(
                    GL_TEXTURE_2D, static_cast<GLint>(i), openGlMeta.internalFormat, static_cast<GLsizei>(mip->width),
                    static_cast<GLsizei>(mip->height), 0, openGlMeta.format, openGlMeta.type, mip->pixels
                );
            }
        }
        else
        {
            const DirectX::Image* baseImage = scratchImage.GetImage(0, 0, 0);
            glTexImage2D(
                GL_TEXTURE_2D, 0, openGlMeta.internalFormat, static_cast<GLsizei>(texMetadata.width),
                static_cast<GLsizei>(texMetadata.height), 0, openGlMeta.format, openGlMeta.type, baseImage->pixels
            );
            glGenerateMipmap(GL_TEXTURE_2D);
        }

        texture->SetTextureID(textureID);

        return texture;
    }

    ResourceTexture* LoadCubemap(UID textureUID)
    {
        const std::string& path     = App->GetLibraryModule()->GetResourcePath(textureUID);

        const std::string& filename = App->GetLibraryModule()->GetResourceName(textureUID);

        unsigned int textureID;
        const std::wstring& wPath   = std::wstring(path.begin(), path.end());

        DirectX::ScratchImage scratchImage;
        DirectX::TexMetadata texMetadata;
        OpenGLMetadata openGlMeta;

        const bool succeded = LoadTextureFile(wPath.c_str(), texMetadata, scratchImage);
        if (!succeded)
        {
            GLOG("Failed to load texture: %s", path.c_str());
            return nullptr;
        }

        ResourceTexture* texture = new ResourceTexture(textureUID, filename);

        texture->LoadData(texMetadata, scratchImage);

        ResourceTexture::ConvertMetadata(texMetadata, openGlMeta);

        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

        // Sending texture to OpenGL
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

        texture->SetTextureID(textureID);

        return texture;
    }

    bool LoadTextureFile(const wchar_t* texturePath, DirectX::TexMetadata& outMetadata, DirectX::ScratchImage& outImage)
    {
        HRESULT hr = LoadFromDDSFile(texturePath, DirectX::DDS_FLAGS_NONE, &outMetadata, outImage);

        if (SUCCEEDED(hr)) return true;

        hr = LoadFromTGAFile(texturePath, DirectX::TGA_FLAGS_NONE, &outMetadata, outImage);

        if (SUCCEEDED(hr)) return true;

        hr = DirectX::LoadFromWICFile(texturePath, DirectX::WIC_FLAGS_NONE, &outMetadata, outImage);

        if (SUCCEEDED(hr)) return true;

        return false;
    }
}; // namespace TextureImporter
