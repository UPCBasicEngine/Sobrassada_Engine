#pragma once

#include "../LightComponent.h"

#include <Libs/rapidjson/document.h>

class DirectionalLight : public LightComponent
{

  public:
    DirectionalLight(UID uid, UID uidParent, UID uidRoot, const Transform &parentGlobalTransform);
    DirectionalLight(const rapidjson::Value& initialState);
    ~DirectionalLight();

    virtual void Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const;

    void RenderEditorInspector() override;
    void Render() override;

    const float3& GetDirection() const { return direction; }

  private:
    float3 direction;
};