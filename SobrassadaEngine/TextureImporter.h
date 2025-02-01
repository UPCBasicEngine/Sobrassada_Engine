#pragma once

namespace TextureImporter
{
	bool Import(const char* filePath);
	const wchar_t* ConvertToWChar(const char* filePath);
};

