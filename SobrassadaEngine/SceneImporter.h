#pragma once

namespace SceneImporter
{
	bool Import(const char* filePath);
	void CreateLibraryDirectories();
	void ImportMeshes(const char* buffer);
};

