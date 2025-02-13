#include "LightComponent.h"

LightComponent::LightComponent()
{
    intensity = 1;
    color     = float3(1.0f, 1.0f, 1.0f);
    drawGizmos = true;
}

LightComponent::~LightComponent() {}