#pragma once
#include "Mesh.h"
#include "Resource.h"
#include "Math/float4x4.h"

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
    ResourceMesh(UID uid, const std::string& name, const float3& maxPos, const float3& minPos);
    ~ResourceMesh() override;

    void SetMaterial(UID materialUID) { this->material = materialUID; }

    void LoadData(unsigned int mode, const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices, float4x4& transform);

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
    UID material      = CONSTANT_EMPTY_UID;

    unsigned int vertexCount = 0;
    unsigned int indexCount  = 0;

    float4x4 transform       = float4x4::identity;

    AABB aabb;
};
