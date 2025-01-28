#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_EXTERNAL_IMAGE

#include "FileSystem.h"
#include "MeshImporter.h"
#include "Utils/Globals.h"
#include "tiny_gltf.h"

namespace SceneImporter {
	void importGLTF(const std::string& filepath) {

		tinygltf::TinyGLTF gltfContext;
		tinygltf::Model model;
		std::string err, warn, name;
		int n = 0;

		bool ret = gltfContext.LoadASCIIFromFile(&model, &err, &warn, filepath);
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
		/*
		//create folder if not exists
		if (!FileSystem::CreateDirectory(outputdirectory.c_sr())) {

		}
		*/
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
}