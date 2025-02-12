#pragma once
#include <vector>
#include "Math/float4x4.h"
#include "Math/float3.h"
#include "Math/float2.h"
#include "ResourceManagement/Resources/ResourceMesh.h"

namespace tinygltf
{
	class Model;
}

class ComponentMaterial;

class EngineModel
{
public:
	EngineModel();
	~EngineModel();

	void Load(const char* modelPath);
	void LoadMaterials(const tinygltf::Model& sourceModel, const char* modelPath);

	void Render(int program, unsigned int cameraUBO);

	float3 GetMaximumValues() const { return maxValues; };
	float3 GetMinimumValues() const { return minValues; };

	int GetIndexCount() const { return indexCount; }
	int GetRenderTexture() const { return renderTexture; };

	std::vector<ComponentMaterial*> &GetMaterials() { return materials; }
    ComponentMaterial &GetMaterial(const int index);

    ResourceMesh* GetMesh(int index) const { return meshes[index]; }

    unsigned int GetActiveRenderTexture() const { return textures.size() > 0 ? renderTexture > -1 ? textures[renderTexture] : textures[textures.size() - 1] : 0; }

private:

	std::vector<ResourceMesh*> meshes;
	std::vector<unsigned int> textures;
	std::vector<float2> textureInfo;
    std::vector<ComponentMaterial*> materials;

	int totalTriangles = 0;
	int renderTexture = -1;

	int indexCount = 0;

	float3 maxValues;
	float3 minValues;

	bool firstMesh = true;

	void LoadRecursive(const tinygltf::Model& sourceModel, const float4x4& parentModelMatrix, int currentNodePosition);
	void ClearVectors();
};

