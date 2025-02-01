#pragma once

#include "Math/float4x4.h"
#include "Math/float3.h"

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

	float3 GetMaximumPosition() const { return maximumPosition; } 
	float3 GetMinimumPosition() const { return minimumPosition; }

	int GetIndexCount() const { return indexCount; }

	void SetBasicModelMatrix(float4x4& newModelMatrix);

	float4x4 GetBasicModelMatrix();

	void Render(int program, int texturePosition, float4x4& projectionMatrix, float4x4& viewMatrix);

private:
	unsigned int vbo = 0;
	unsigned int ebo = 0;
	unsigned int vao = 0;

	int vertexCount = 0;
	int textureCoordCount = 0;
	unsigned int indexCount = 0; // Return indexCount/3 -> numero de triangles per mesh

	float4x4 basicModelMatrix;

	float3 maximumPosition;
	float3 minimumPosition;
};

