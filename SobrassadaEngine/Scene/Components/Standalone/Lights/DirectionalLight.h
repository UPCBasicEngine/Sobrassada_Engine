#pragma once

#include "../LightComponent.h"

class DirectionalLight : public LightComponent
{

  public:
    DirectionalLight(UID uid, UID uidParent, UID uidRoot, const Transform &parentGlobalTransform);
    ~DirectionalLight();

    void RenderEditorInspector() override;

    float3 GetDirection() const { return direction; }

  private:
    float3 direction;
};