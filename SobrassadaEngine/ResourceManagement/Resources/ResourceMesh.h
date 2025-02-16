#pragma once
#include "Mesh.h"
#include "Resource.h"

#include <Geometry/AABB.h>

class ResourceMaterial;
namespace tinygltf
{
    class Model;
    struct Mesh;
    struct Primitive;
} // namespace tinygltf

class ResourceMesh : public Resource
{
  public:
    ResourceMesh(UID uid, const std::string& name);
    ~ResourceMesh() override;

    void LoadData(unsigned int mode, const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices);

    void LoadVBO(const tinygltf::Model& inModel, const tinygltf::Mesh& inMesh, const tinygltf::Primitive& inPrimitive);
    void LoadEBO(const tinygltf::Model& inModel, const tinygltf::Mesh& inMesh, const tinygltf::Primitive& inPrimitive);
    void CreateVAO();

    int GetIndexCount() const { return indexCount; }

    void Render(int program, float4x4& modelMatrix, unsigned int cameraUBO, ResourceMaterial* material);

    const AABB& GetAABB() const { return aabb; }

  private:
    unsigned int vbo         = 0;
    unsigned int ebo         = 0;
    unsigned int vao         = 0;

    unsigned int mode        = 0;
    UID defaultMaterial      = CONSTANT_EMPTY_UID;

    unsigned int vertexCount = 0;
    unsigned int indexCount  = 0;

    AABB aabb;
};
