#include "SceneImporter.h"

#include "TextureImporter.h"
#include "FileSystem.h"
#include "MeshImporter.h"
#include "Globals.h"

#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_EXTERNAL_IMAGE
#include "tiny_gltf.h"

namespace SceneImporter {
	void Import(const char* filePath)
	{
		CreateLibraryDirectories();

		std::string extension = FileSystem::GetFileExtension(filePath);

		// Copy file to Assets folder
		{
			std::string copyPath = "Assets/" + FileSystem::GetFileNameWithExtension(filePath);
			if (!FileSystem::Exists(copyPath.c_str()))
			{
				FileSystem::Copy(filePath, copyPath.c_str());
			}
		}

		if (extension == ".gltf")
		{
			ImportGLTF(filePath);

		}
		else if (extension != ".gltf")
		{
			TextureImporter::Import(filePath);
		}
	}

	void ImportGLTF(const char* filePath) {

		tinygltf::TinyGLTF gltfContext;
		tinygltf::Model model;
		std::string err, warn, name;
		int n = 0;

		bool ret = gltfContext.LoadASCIIFromFile(&model, &err, &warn, std::string(filePath));
		if (!ret)
		{
			if (!warn.empty()) {
				GLOG("Warn: %s\n", warn.c_str());
			}

			if (!err.empty()) {
				GLOG("Err: %s\n", err.c_str());
			}

			if (!ret) {
				GLOG("Failed to parse glTF\n");
			}
		}

		for (const auto& srcMesh : model.meshes)
		{
			name = srcMesh.name;
			n = 0;
			for (const auto& primitive : srcMesh.primitives)
			{
				name += std::to_string(n);
				MeshImporter::ImportMesh(model, srcMesh, primitive, name);
				n++;
			}
		}
	}

	void CreateLibraryDirectories()
	{
		if (!FileSystem::IsDirectory("Assets"))
		{
			if (!FileSystem::CreateDirectories("Assets")) {
				GLOG("Failed to create directory: Assets");
			}
		}
		if (!FileSystem::IsDirectory("Library/Meshes"))
		{
			if (!FileSystem::CreateDirectories("Library/Meshes")) {
				GLOG("Failed to create directory: Library/Meshes");
			}
		}
		if (!FileSystem::IsDirectory("Library/Materials"))
		{
			if (!FileSystem::CreateDirectories("Library/Materials")) {
				GLOG("Failed to create directory: Library/Materials");
			}
		}
	}
};