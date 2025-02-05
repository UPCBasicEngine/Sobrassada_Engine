#include "TextureModuleTest.h"

#include "DirectXTex/DirectXTex.h"
#include "glew.h"
#include <string>
#include <TextureImporter.h>

TextureModuleTest::TextureModuleTest()
{
}

TextureModuleTest::~TextureModuleTest()
{
}

unsigned int TextureModuleTest::LoadTexture(const char* texturePath, DirectX::TexMetadata& outTexMetadata)
{
	GLOG("Loading texture: %s", texturePath)
		unsigned int textureId = 0;

	DirectX::ScratchImage scratchImage;
	OpenGLMetadata openGlMeta;

	const wchar_t* wPath = TextureImporter::ConvertToWChar(texturePath);
	bool succeded = LoadTextureFile(wPath, outTexMetadata, scratchImage);
	delete[] wPath;
	if (succeded)
	{
		ConvertMetadata(outTexMetadata, openGlMeta);

		glGenTextures(1, &textureId);
		glBindTexture(GL_TEXTURE_2D, textureId);

		// Sending texture to OpgenGL
		glTexImage2D(GL_TEXTURE_2D, 0, openGlMeta.internalFormat, outTexMetadata.width, outTexMetadata.height, 0, openGlMeta.format, openGlMeta.type, scratchImage.GetPixels());
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	}

	return textureId;
}

void TextureModuleTest::ConvertMetadata(const DirectX::TexMetadata& metadata, OpenGLMetadata& outMetadata)
{
	switch (metadata.format)
	{
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
	case DXGI_FORMAT_R8G8B8A8_UNORM:
		outMetadata.internalFormat = GL_RGBA8;
		outMetadata.format = GL_RGBA;
		outMetadata.type = GL_UNSIGNED_BYTE;
		break;
	case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
	case DXGI_FORMAT_B8G8R8A8_UNORM:
		outMetadata.internalFormat = GL_RGBA8;
		outMetadata.format = GL_BGRA;
		outMetadata.type = GL_UNSIGNED_BYTE;
		break;
	case DXGI_FORMAT_B5G6R5_UNORM:
		outMetadata.internalFormat = GL_RGB8;
		outMetadata.format = GL_BGR;
		outMetadata.type = GL_UNSIGNED_BYTE;
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

bool TextureModuleTest::LoadTextureFile(const wchar_t* texturePath, DirectX::TexMetadata& outMetadata, DirectX::ScratchImage& outImage)
{
	HRESULT hr = LoadFromDDSFile(texturePath, DirectX::DDS_FLAGS_NONE, &outMetadata, outImage);

	if (SUCCEEDED(hr)) return true;

	hr = LoadFromTGAFile(texturePath, DirectX::TGA_FLAGS_NONE, &outMetadata, outImage);

	if (SUCCEEDED(hr)) return true;

	hr = DirectX::LoadFromWICFile(texturePath, DirectX::WIC_FLAGS_NONE, &outMetadata, outImage);

	if (SUCCEEDED(hr)) return true;

	return false;
}
