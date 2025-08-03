#pragma once

#include "Component.h"
#include "Utils/EventDispatcher.h"

#include "Math/float3.h"

class Transform2DComponent;
class CanvasComponent;
class ImageComponent;

class SOBRASADA_API_ENGINE ButtonComponent : public Component
{
  public:
    ButtonComponent(UID uid, GameObject* parent);
    ButtonComponent(const rapidjson::Value& initialState, GameObject* parent);
    ~ButtonComponent() override;

    void Init() override;
    void Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const override;
    void Clone(const Component* other) override;
    void Update(float deltaTime) override {};
    void RenderDebug(float deltaTime) override;
    void RenderEditorInspector() override;

    bool UpdateMousePosition(const float2& mousePos, bool dismiss = false);
    void OnClick();
    void OnRelease() const;

    std::list<Delegate<void>>::iterator AddOnClickCallback(Delegate<void> newDelegate);
    void RemoveOnClickCallback(std::list<Delegate<void>>::iterator delegate);
    void RemoveTransform() { transform2D = nullptr; }
    void ClearAllCallbacks();

  private:
    bool IsWithinBounds(const float2& pos) const;
    void OnInteractionChange() const;

  private:
    Transform2DComponent* transform2D;
    CanvasComponent* parentCanvas;
    ImageComponent* image;

    float3 defaultColor;
    float3 hoverColor;
    float3 clickedColor;
    float3 disabledColor;

    bool isHovered;
    bool isInteractable;
    EventDispatcher<void> onClickDispatcher;
    std::list<Delegate<void>>::iterator delegateID;
};
