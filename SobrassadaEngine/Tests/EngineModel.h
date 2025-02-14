#pragma once
#include <vector>
#include "Math/float4x4.h"
#include "Math/float3.h"
#include "Math/float2.h"

namespace tinygltf
{
	class Model;
}

class ResourceMaterial;
class ResourceMesh;

class EngineModel
{
public:
	EngineModel();
	~EngineModel();

	void Load(const char* modelPath);
	void LoadMaterials(const tinygltf::Model& sourceModel, const char* modelPath);


	void LoadAdditionalTexture(const char* texturePath);

	void Render(int program, float4x4& modelMatrix, float4x4& projectionMatrix, float4x4& viewMatrix);

	float3 GetMaximumValues() const { return maxValues; };
	float3 GetMinimumValues() const { return minValues; };

	int GetIndexCount() const { return indexCount; }
	int GetRenderTexture() const { return renderTexture; };
	void GetTextureSize(float2& outTextureSize);
    OBB GetOBBModel() const;
    AABB GetABBModel() const;
	void SetRenderTexture(int texturePosition);


    EngineMesh* GetMesh(int index) const { return meshes[index]; }

    unsigned int GetActiveRenderTexture() const { return textures.size() > 0 ? renderTexture > -1 ? textures[renderTexture] : textures[textures.size() - 1] : 0; }

private:

	std::vector<ResourceMesh*> meshes;
	std::vector<unsigned int> textures;
	std::vector<float2> textureInfo;
    std::vector<ResourceMaterial*> materials;

	int totalTriangles = 0;
	int renderTexture = -1;

	int indexCount = 0;

	float3 maxValues;
	float3 minValues;

	bool firstMesh = true;

	void LoadRecursive(const tinygltf::Model& sourceModel, const float4x4& parentModelMatrix, int currentNodePosition);
	void ClearVectors();
};

