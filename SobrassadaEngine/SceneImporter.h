#pragma once

#include <string>
namespace SceneImporter
{
	bool Import(const char* filePath);
	void CreateLibraryDirectories();
	void ImportMeshes(const char* buffer);
};

namespace SceneImporter {
	void importGLTF(const std::string& filepath);
}