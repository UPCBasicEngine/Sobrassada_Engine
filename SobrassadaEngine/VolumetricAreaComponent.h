#pragma once

#include "Component.h"

#include "Math/float3.h"

static float3 VolumetricAreaDebugColor = float3(0.f, 0.467f, 1.f);

class VolumetricAreaComponent : public Component
{
  public:
    VolumetricAreaComponent(UID uid, GameObject* parent);
    VolumetricAreaComponent(const rapidjson::Value& initialState, GameObject* parent);

    ~VolumetricAreaComponent() override;

    void Init() override;
    void Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const override;
    void Clone(const Component* other) override;

    void RenderEditorInspector() override;

    void Update(float deltaTime) override;
    void RenderDebug(float deltaTime) override;


    const float3& GetSize() const { return size; }
    VolumetricAreaType GetAreaType() const { return volumeType; }

  private:
    // USE X COORDINATE FOR RADIUS IN CASE OF BEING A SPHERE
    float3 size = float3::one;
    VolumetricAreaType volumeType = VolumetricAreaType::BOX;

};
