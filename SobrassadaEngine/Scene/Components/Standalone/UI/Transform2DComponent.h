#pragma once

#include "Component.h"
#include "Math/float2.h"
#include "Math/float4.h"

class CanvasComponent;

class Transform2DComponent : public Component
{
  public:
    static const ComponentType STATIC_TYPE = ComponentType::COMPONENT_TRANSFORM_2D;

    Transform2DComponent(UID uid, GameObject* parent);
    Transform2DComponent(const rapidjson::Value& initialState, GameObject* parent);
    ~Transform2DComponent() override;

    void Init() override;
    void Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const override;
    void Clone(const Component* other) override;

    void Update(float deltaTime) override {};
    void Render(float deltaTime) override {};
    void RenderDebug(float deltaTime) override;
    void RenderEditorInspector() override;

    void RenderWidgets() const;
    void UpdateParent3DTransform();
    void OnTransform3DUpdated(const float4x4& transform3D);
    void OnParentChange();
    void GetCanvas();
    void AdaptToParentChanges();

    float2 GetRenderingPosition() const;
    float2 GetGlobalPosition() const;
    float2 GetCenterPosition() const;
    void AddChildTransform(Transform2DComponent* newChild) { childTransforms.push_back(newChild); }
    void RemoveChild(Transform2DComponent* child);
    void RemoveParent() { parentTransform = nullptr; }
    CanvasComponent* GetParentCanvas() const { return parentCanvas; }

  private:
    bool IsRootTransform2D() const;

    float GetAnchorXPos(const float anchor) const;
    float GetAnchorYPos(const float anchor) const;

    void OnAnchorsUpdated();
    void OnSizeChanged();
    void UpdateHorizontalMargins();
    void UpdateVerticalMargins();

    void OnLeftMarginChanged();
    void OnRightMarginChanged();
    void OnTopMarginChanged();
    void OnBottomMarginChanged();

  public:
    float2 position;
    float2 size;
    float2 pivot;
    float2 anchorsX;
    float2 anchorsY;

  private:
    CanvasComponent* parentCanvas;
    Transform2DComponent* parentTransform;
    std::vector<Transform2DComponent*> childTransforms;

    bool transform2DUpdated;
    bool renderAnchors = true;
    float2 previousPosition;
    float4 previousMargins;
    float4 margins;
};