#pragma once

#include "Component.h"

class VideoComponent : public Component
{
  public:
    VideoComponent(UID uid, GameObject* parent);
    VideoComponent(const rapidjson::Value& initialState, GameObject* parent);
    ~VideoComponent() override;

    void Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const override;
    void Clone(const Component* other) override;

    void Update(float deltaTime) override;
    void Render(float deltaTime) override;
    void RenderDebug(float deltaTime) override;
    void RenderEditorInspector() override;
    void ParentUpdated() override;
};