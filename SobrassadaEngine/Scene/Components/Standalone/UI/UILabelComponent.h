#pragma once

#include "Component.h"

#include "Math/float3.h"

namespace TextManager
{
    struct FontData;
}

namespace Math
{
    class float4x4;
}

class Transform2DComponent;
class CanvasComponent;
class ResourceFont;

class UILabelComponent : public Component
{
  public:
    UILabelComponent(UID uid, GameObject* parent);
    UILabelComponent(const rapidjson::Value& initialState, GameObject* parent);
    ~UILabelComponent() override;

    void Init() override;
    void Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const override;
    void Clone(const Component* other) override;
    void Update(float deltaTime) override {};
    void RenderDebug(float deltaTime) override;
    void RenderEditorInspector() override;

    void RenderUI(const float4x4& view, const float4x4 proj) const;
    void RemoveTransform() { transform2D = nullptr; }

  private:
    void InitBuffers();
    void OnFontChange();

  private:
    Transform2DComponent* transform2D;
    TextManager::FontData* fontData;

    char text[512];
    int fontSize = 48;
    float3 fontColor;
    ResourceFont* fontType;

    unsigned int vbo = 0;
    unsigned int vao = 0;

    CanvasComponent* parentCanvas;
};