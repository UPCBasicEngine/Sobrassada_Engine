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
}

class TextureModuleTest : public Module
{
public:
	TextureModuleTest();
	~TextureModuleTest();

	unsigned int LoadTexture(const wchar_t* texturePath, DirectX::TexMetadata& outTexMetadata);
	static void ConvertMetadata(const DirectX::TexMetadata& metadata, OpenGLMetadata& outMetadata);
private:
	bool LoadTextureFile(const wchar_t* texturePath, DirectX::TexMetadata& outMetadata, DirectX::ScratchImage& outImage);
};

