#pragma once

#include "Component.h"
#include "Math/float2.h"

class CanvasComponent;
class Transform2DComponent;

class CanvasScalerComponent : public Component
{
  public:
    static const ComponentType STATIC_TYPE = ComponentType::COMPONENT_CANVAS_SCALER;

    CanvasScalerComponent(UID uid, GameObject* parent);
    CanvasScalerComponent(const rapidjson::Value& initialState, GameObject* parent);

    void Init() override;
    void Update(float deltaTime) override;
    void Render(float deltaTime) override {};
    void RenderDebug(float deltaTime) override {};
    void Save(rapidjson::Value& state, rapidjson::Document::AllocatorType& allocator) const override;
    void Clone(const Component* other) override {};
    void RenderEditorInspector() override;

    float ComputeScale(float2 windowSize) const;
    float GetScale() const { return scale; }

  private:
    float2 referenceResolution        = float2(1920.0f, 1080.0f);

    CanvasComponent* canvas           = nullptr;
    Transform2DComponent* transform2D = nullptr;

    float scale                       = 1.0f;
};
