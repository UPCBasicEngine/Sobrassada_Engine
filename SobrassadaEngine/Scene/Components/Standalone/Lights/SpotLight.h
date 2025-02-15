#pragma once

#include "../LightComponent.h"

class SpotLight : public LightComponent
{
  public:
    SpotLight(UID uid, UID uidParent, UID uidRoot, const Transform &parentGlobalTransform);
    //SpotLight(const float3 &position, const float3 &direction);
    ~SpotLight();

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