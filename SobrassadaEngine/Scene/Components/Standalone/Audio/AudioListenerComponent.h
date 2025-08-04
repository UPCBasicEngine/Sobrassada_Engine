#pragma once

#include "Component.h"

class AudioListenerComponent : public Component
{
  public:
    AudioListenerComponent(UID uid, GameObject* parent);
    AudioListenerComponent(const rapidjson::Value& initialState, GameObject* parent);
    ~AudioListenerComponent() override;

    void Init() override;
    void Update(float deltaTime) override {};
    void Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const override;
    void Clone(const Component* other) override;

    void RenderEditorInspector() override;
    virtual void RenderDebug(float deltaTime) {};

  private:
    bool isActiveListener;
};
