#include "TextureImporter.h"

#include "DirectXTex/DirectXTex.h"
#include "FileSystem.h"
#include "Globals.h"



namespace TextureImporter
{
    std::string Import(const char *filePath)
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
                    return false;
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
            return false;
        }

        std::string fileName = FileSystem::GetFileNameWithoutExtension(filePath);
        std::string savePath = MATERIALS_PATH + fileName + MATERIAL_EXTENSION;

        unsigned int size =
            FileSystem::Save(savePath.c_str(), blob.GetBufferPointer(), (unsigned int)blob.GetBufferSize());

        if (size == 0)
        {
            GLOG("Failed to save DDS file: %s", savePath.c_str());
            return false;
        }

        GLOG("%s saved as dds", fileName.c_str());

        return savePath.c_str();
    }
};
