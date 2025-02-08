#include "LightComponent.h"

LightComponent::LightComponent()
{
    intensity = 0;
    color     = float4(1.0f, 1.0f, 1.0f, 1.0f);
}

LightComponent::~LightComponent() {}