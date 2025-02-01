#include "TextureImporter.h"

#include "FileSystem.h"
#include "DirectXTex/DirectXTex.h"
#include "Globals.h"

#include <string>

namespace TextureImporter
{
	bool Import(const char* filePath)
	{
		const wchar_t* wPath = ConvertToWChar(filePath);

		DirectX::ScratchImage image;
		HRESULT hr = DirectX::LoadFromWICFile(wPath, DirectX::WIC_FLAGS_NONE, nullptr, image);

		delete[] wPath;

		if (FAILED(hr))
		{
			GLOG("Failed to load texture: %s", filePath);
			return false;
		}

		DirectX::Blob blob;
		hr = DirectX::SaveToDDSMemory(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::DDS_FLAGS_NONE, blob);

		if (FAILED(hr))
		{
			GLOG("Failed to convert texture to DDS: %s", filePath);
			return false;
		}

		std::string fileName = FileSystem::GetFileNameWithoutExtension(filePath);
		std::string savePath = "Library/Materials/" + fileName + ".dds";

		unsigned int size = FileSystem::Save(savePath.c_str(), blob.GetBufferPointer(), static_cast<unsigned int>(blob.GetBufferSize()));

		if (size == 0)
		{
			GLOG("Failed to save DDS file: %s", savePath.c_str());
			return false;
		}

		GLOG("%s saved as dds", fileName.c_str());
		return true;
	}

	const wchar_t* ConvertToWChar(const char* filePath) {
		size_t len = 0;
		mbstowcs_s(&len, nullptr, 0, filePath, 0);

		wchar_t* wImagePath = new wchar_t[len + 1];

		mbstowcs_s(&len, wImagePath, len + 1, filePath, _TRUNCATE);

		return wImagePath;
	}
};
