#include "LightComponent.h"

LightComponent::LightComponent()
{
    intensity = 0;
    color     = float3(1.0f, 1.0f, 1.0f);
}

LightComponent::~LightComponent() {}