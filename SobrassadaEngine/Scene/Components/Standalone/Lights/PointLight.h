#pragma once

#include "../LightComponent.h"

class PointLight : public LightComponent
{
  public:
    PointLight(UID uid, UID uidParent, UID uidRoot, const Transform &parentGlobalTransform);
    //PointLight(const float3 &position, const float range);
    ~PointLight();

    void RenderEditorInspector() override;
    void Render() override;

    float GetRange() const { return range; }

  private:
    float range;
    int gizmosMode;
};
