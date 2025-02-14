#pragma once
#include "Resource.h"

#include <Geometry/AABB.h>


class ResourceMaterial;namespace tinygltf
{
class Model;
struct Mesh;
struct Primitive;
}

class ResourceMesh : public Resource
{
public:

    ResourceMesh(UID uid);
    ~ResourceMesh() override;

    void LoadVBO(const tinygltf::Model& inModel, const tinygltf::Mesh& inMesh, const tinygltf::Primitive& inPrimitive);
    void LoadEBO(const tinygltf::Model& inModel, const tinygltf::Mesh& inMesh, const tinygltf::Primitive& inPrimitive);
    void CreateVAO();

    int GetIndexCount() const { return indexCount; }

    void Render(int program, float4x4& modelMatrix, unsigned int cameraUBO, ResourceMaterial* material);

    const AABB& GetAABB() const { return aabb;}

private:
    unsigned int vbo = 0;
    unsigned int ebo = 0;
    unsigned int vao = 0;

    int vertexCount = 0;
    int textureCoordCount = 0;
    int normalCoordCount = 0;
    unsigned int indexCount = 0; // Return indexCount/3 -> numero de triangles per mesh

    AABB aabb;
};
