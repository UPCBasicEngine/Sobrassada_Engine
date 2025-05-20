#pragma once

enum class ParticleAddonType : int
{
    NONE = 0,
    BASE,
    VELOCITY,
};

#define ADDON_TYPES BaseAddon*, VelocityAddon*

#define ADDON_NULLPTR nullptr, nullptr