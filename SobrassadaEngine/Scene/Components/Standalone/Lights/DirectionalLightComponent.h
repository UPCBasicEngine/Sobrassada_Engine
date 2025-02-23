#pragma once

#include "../LightComponent.h"

#include <Libs/rapidjson/document.h>

class DirectionalLightComponent : public LightComponent
{

  public:
    DirectionalLightComponent(UID uid, UID uidParent, UID uidRoot, const Transform &parentGlobalTransform);
    DirectionalLightComponent(const rapidjson::Value& initialState);
    ~DirectionalLightComponent();

    virtual void Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const;

    void RenderEditorInspector() override;
    void Render() override;

    float3 GetDirection() const { return direction; }

  private:
    float3 direction;
};