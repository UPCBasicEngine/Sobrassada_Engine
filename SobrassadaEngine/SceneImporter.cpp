#include "SceneImporter.h"

#include "FileSystem.h"
#include "Globals.h"
#include "MeshImporter.h"
#include "TextureImporter.h"

#include <string>

namespace SceneImporter
{
	bool Import(const char* filePath)
	{
		CreateLibraryDirectories();

		std::string extension = FileSystem::GetFileExtension(filePath);

		{
			std::string copyPath = "Assets/" + FileSystem::GetFileNameWithExtension(filePath);
			if (!FileSystem::Exists(copyPath.c_str()))
			{
				FileSystem::Copy(filePath, copyPath.c_str());
			}
		}

		if (extension != ".gltf")
		{
			return TextureImporter::Import(filePath);
		}

		char* buffer = nullptr;

		unsigned int size = FileSystem::Load(filePath, &buffer);

		if (size == 0)
		{
			GLOG("Failed to load: %s", filePath);
			return false;
		}

		ImportMeshes(buffer);

		delete[] buffer;
		return true;
	}

	void CreateLibraryDirectories()
	{
		if (!FileSystem::CreateDirectories("Assets")) {
			//GLOG("Failed to create directory: Assets");
		}
		if (!FileSystem::CreateDirectories("Library/Meshes")) {
			//GLOG("Failed to create directory: Library/Meshes");
		}
		if (!FileSystem::CreateDirectories("Library/Materials")) {
			//GLOG("Failed to create directory: Library/Materials");
		}
	}

	void ImportMeshes(const char* buffer)
	{
		MeshHeader header;
	}
};