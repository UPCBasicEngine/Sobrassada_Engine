#pragma once

#include "debug_draw.hpp"
#include <vector>
#include "Math/float4x4.h"
#include "Math/float3.h"
#include "Components/ComponentMaterial.h"

#include <Geometry/AABB.h>

namespace tinygltf
{
	class Model;
	struct Mesh;
	struct Primitive;
}

class EngineMesh
{
public:
	EngineMesh();
	~EngineMesh();

	void LoadVBO(const tinygltf::Model& inModel, const tinygltf::Mesh& inMesh, const tinygltf::Primitive& inPrimitive);
	void LoadEBO(const tinygltf::Model& inModel, const tinygltf::Mesh& inMesh, const tinygltf::Primitive& inPrimitive);
	void CreateVAO();

	int GetIndexCount() const { return indexCount; }

	void SetBasicModelMatrix(float4x4& newModelMatrix);
    void SetMaterialIndex(int index) { materialIndices.push_back(index); }
	std::vector<int>& GetMaterialIndices() {  return materialIndices; }

	void Render(int program, ComponentMaterial* material, float4x4& modelMatrix, unsigned int cameraUBO);

    const AABB& GetAABB() const { return aabb;}

private:
    
	unsigned int vbo = 0;
	unsigned int ebo = 0;
	unsigned int vao = 0;

	int vertexCount = 0;
	int textureCoordCount = 0;
    int normalCoordCount = 0;
	unsigned int indexCount = 0; // Return indexCount/3 -> numero de triangles per mesh

	float4x4 basicModelMatrix;

    AABB aabb;

    std::vector<int> materialIndices;
};

