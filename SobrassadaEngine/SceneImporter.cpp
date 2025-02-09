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

		if (extension == ".gltf")
			ImportGLTF(filePath);
		else
			TextureImporter::Import(filePath);
	}

	void ImportGLTF(const char* filePath) 
	{
		// Copy gltf to Assets folder
		{
			std::string copyPath = ASSETS_PATH + FileSystem::GetFileNameWithExtension(filePath);
			if (!FileSystem::Exists(copyPath.c_str()))
			{
				FileSystem::Copy(filePath, copyPath.c_str());
			}
		}

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

		std::string path = FileSystem::GetFilePath(filePath);

		// Copy bin to Assets folder
		for (const auto& srcBuffers : model.buffers)
		{
			std::string binPath = path + srcBuffers.uri;
			std::string copyPath = ASSETS_PATH + FileSystem::GetFileNameWithExtension(binPath);
			if (!FileSystem::Exists(copyPath.c_str()))
			{
				FileSystem::Copy(binPath.c_str(), copyPath.c_str());
			}
		}

		for (const auto& srcImages : model.images)
		{
			std::string fullPath = path + srcImages.uri;
			TextureImporter::Import(fullPath.c_str());
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