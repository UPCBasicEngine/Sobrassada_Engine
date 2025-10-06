#pragma once

#include "Component.h"
#include "Globals.h"

#include "Math/float4x4.h"
#include "rapidjson/document.h"
#include <cstdint>

class ResourceMesh;
class ResourceMaterial;
class GeometryBatch;

class SOBRASADA_API_ENGINE MeshComponent : public Component
{
  public:
    MeshComponent(UID uid, GameObject* parent);

    MeshComponent(const rapidjson::Value& initialState, GameObject* parent);

    ~MeshComponent() override;

    void Init() override;
    void Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const override;
    void Clone(const Component* other) override;

    void RenderEditorInspector() override;

    void Update(float deltaTime) override;
    void RenderDebug(float deltaTime) override;

    void InitSkin();
    void OnTransformUpdated();

    void BatchEditorMode();

    const ResourceMesh* GetResourceMesh() const { return currentMesh; }
    const ResourceMaterial* GetResourceMaterial() const { return currentMaterial; }

    void AddMesh(UID resource, bool updateParent = true);
    void AddMaterial(UID resource, bool setDefaultMaterial = false);

    const bool GetHasBones() const { return hasBones; }
    const std::vector<GameObject*>& GetBonesGO() const { return bones; }
    const std::vector<UID>& GetBones() const { return bonesUIDs; }
    const std::vector<float4x4>& GetBindMatrices() const { return bindMatrices; }
    const float4x4& GetCombinedMatrix() const { return combinedMatrix; }
    GeometryBatch* GetBatch() const { return batch; }
    int GetRenderMode() const { return renderMode; }
    bool GetProduceShadows() const { return produceShadows; }
    bool GetBatchWasEnabled() { return batchWasEnabled; }

    unsigned int GetBoneIndexOffset() const { return boneIndexOffset; }
    bool GetUpdateShaderStorage() const { return updateShaderStorage; }
    bool GetAdditive() const { return additive; }

    void SetBones(const std::vector<GameObject*>& bones, const std::vector<UID> bonesIds)
    {
        this->bones     = bones;
        this->bonesUIDs = bonesIds;
    }
    void SetBindMatrices(const std::vector<float4x4>& bindTransforms) { this->bindMatrices = bindTransforms; }
    void SetModelUID(const UID newModelUID) { this->modelUID = newModelUID; }
    void SetSkinIndex(const int newIndex)
    {
        this->skinIndex = newIndex;
        hasBones        = true;
    }
    void SetBatchWasEnabled() { batchWasEnabled = true; }

    void SetEnabled(bool newEnabled) override
    {
        Component::SetEnabled(newEnabled);

        batchWasEnabled = false;
    }

    void SetBoneIndexOffset(unsigned int newIndexOffset) { boneIndexOffset = newIndexOffset; }
    void SetUpdateShaderStorage(bool newUpdate) { updateShaderStorage = newUpdate; }

  private:
    std::string currentMeshName       = "Not selected";
    ResourceMesh* currentMesh         = nullptr;

    std::string currentMaterialName   = "Not selected";
    ResourceMaterial* currentMaterial = nullptr;
    bool bUsesMeshDefaultMaterial     = true;

    std::vector<UID> bonesUIDs;
    std::vector<GameObject*> bones;
    std::vector<float4x4> bindMatrices;
    bool hasBones                = false;

    UID modelUID                 = INVALID_UID;
    int skinIndex                = -1;

    float4x4 combinedMatrix      = float4x4::identity;

    GeometryBatch* batch         = nullptr;
    bool uniqueBatch             = false;

    int renderMode               = 0; // 0 = Opaque, 1 = Alpha Blend, 2 = Alpha Discard

    bool produceShadows          = true;
    bool additive                = false;

    bool batchWasEnabled         = false;

    unsigned int boneIndexOffset = 0;
    bool updateShaderStorage     = false;
};
