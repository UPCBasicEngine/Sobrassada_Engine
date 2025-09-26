#pragma once

#include "Component.h"

#include "Math/float3.h"

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

  private:
    // USE X COORDINATE FOR RADIUS IN CASE OF BEING A SPHERE
    float3 size = float3::one;
};
