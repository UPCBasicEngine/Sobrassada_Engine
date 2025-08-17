#pragma once

#include "Globals.h"

#include "Math/float4x4.h"
#include <unordered_map>
#include <vector>

class MeshComponent;
class ResourceMesh;
class MeshComponent;
struct Command;
struct MaterialGPU;
typedef struct __GLsync* GLsync;
typedef unsigned int GLuint;

struct AccMeshCount
{
    unsigned int accVertexCount;
    unsigned int accIndexCount;
};

class GeometryBatch
{
  public:
    GeometryBatch(const MeshComponent* component);
    ~GeometryBatch();

    void LoadData();
    void Render(const std::vector<MeshComponent*>& meshesToRender, bool shadowMap = false);

    void AddComponent(const MeshComponent* component) { components.push_back(component); }

    const unsigned int GetMode() const { return mode; }
    const bool GetIsMetallic() const { return isMetallic; }
    const bool GetIsSpecular() const { return isSpecular; }
    const bool GetHasBones() const { return hasBones; }
    const bool IsNavmeshValid() const { return isNavmeshValid; }
    const bool IsTransparent() const { return isTransparent; }
    const bool IsAlpha() const { return isAlpha; }
    const bool IsDoubleSided() const { return isDoubleSided; }
    const bool DoApplyWind() const { return doApplyWind; }
    const float GetVCoord0() const { return vCoord0; }
    const float GetVCoord1() const { return vCoord1; }
    const bool UseCentralPivot() const { return useCentralPivot; }
    const bool UseWindGravity() const { return useWindGravity; }
    const unsigned int GetVertexCount() const { return totalVertexCount; }
    const unsigned int GetIndexCount() const { return totalIndexCount; }
    void ResetUpdatedOnce() { updatedOnce = false; }

    void SOBRASADA_API_ENGINE BindBonesBuffer();
    void SOBRASADA_API_ENGINE UnbindBonesBuffer();

  private:
    void LockBuffer();
    void UpdateBuffers(const std::vector<MeshComponent*>& meshesToRender);
    void WaitBuffer();

    void GenerateCommands(const std::vector<MeshComponent*>& meshes, std::vector<Command>& commands);

    void CleanUp();

  private:
    std::vector<const MeshComponent*> components;
    std::unordered_map<const MeshComponent*, std::size_t> componentsMap; // index of position added

    std::unordered_map<const ResourceMesh*, std::size_t> uniqueMeshesMap;
    std::vector<AccMeshCount> uniqueMeshesCount;

    bool updatedOnce           = false;
    GLsync gSync[2]            = {nullptr, nullptr};
    int currentBufferIndex     = 0;

    GLuint models[2]           = {0, 0};
    float4x4* ptrModels[2]     = {nullptr, nullptr};
    std::size_t modelsSize     = 0;

    GLuint bones[2]            = {0, 0};
    GLuint bonesIndex          = 0;
    float4x4* ptrBones[2]      = {nullptr, nullptr};
    std::size_t bonesSize      = 0;
    std::size_t bonesIndexSize = 0;
    std::vector<unsigned int> bonesCount;

    unsigned int totalVertexCount = 0;
    unsigned int totalIndexCount  = 0;

    unsigned int indirect         = 0;
    unsigned int vao              = 0;
    unsigned int vbo              = 0;
    unsigned int ebo              = 0;
    unsigned int materials        = 0;

    unsigned int mode             = 0;
    bool isMetallic               = false;
    bool isSpecular               = false;
    bool hasBones                 = false;
    bool isNavmeshValid           = false;
    bool isTransparent            = false;
    bool isAlpha                  = false;
    bool isDoubleSided            = false;
    bool doApplyWind              = false;
    float vCoord0 = 0.0f;
    float vCoord1 = 1.0f;
    bool useCentralPivot = false;
    bool useWindGravity          = false;
};
