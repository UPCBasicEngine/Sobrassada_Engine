#pragma once

#include "Component.h"
#include "ResourceTexture.h"

class Transform2DComponent;
class CanvasComponent;

class ImageComponent : public Component
{
  public:
    ImageComponent(UID uid, GameObject* parent);
    ImageComponent(const rapidjson::Value& initialState, GameObject* parent);
    ~ImageComponent() override;

    void Init() override;
    void Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const override;
    void Clone(const Component* other) override;
    void Update(float deltaTime) override;
    void Render(float deltaTime) override {};
    void RenderDebug(float deltaTime) override;
    void RenderEditorInspector() override;

    void RenderUI(const float4x4& view, const float4x4& proj) const;
    void RemoveTransform() { transform2D = nullptr; }
    void SetColor(const float3& newColor) { color = newColor; }

  private:
    void InitBuffers();
    void ClearBuffers() const;
    void ChangeTexture(const UID textureUID);

  private:
    Transform2DComponent* transform2D;
    CanvasComponent* parentCanvas;

    ResourceTexture* texture;
    float3 color;

    unsigned int vbo     = 0;
    unsigned int vao     = 0;
    UID bindlessUID;
};
