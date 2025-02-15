#pragma once

#include "../LightComponent.h"

#include <Libs/rapidjson/document.h>

class SpotLight : public LightComponent
{
  public:
    SpotLight(UID uid, UID uidParent, UID uidRoot, const Transform &parentGlobalTransform);
    SpotLight(const rapidjson::Value& initialState);
    ~SpotLight();

    virtual void Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const;

    void RenderEditorInspector() override;
    void Render() override;

    float3 GetDirection() const { return direction; }
    float GetRange() const { return range; }
    float GetInnerAngle() const { return innerAngle; }
    float GetOuterAngle() const { return outerAngle; }

  private:
    float3 direction;
    float range;
    float innerAngle;
    float outerAngle;
};