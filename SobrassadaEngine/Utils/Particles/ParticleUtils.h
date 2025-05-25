#pragma once

#include "rapidjson/document.h"

class ParticleEmitter;

enum class ParticleAddonType : int
{
    NONE = 0,
    BASE,
    VELOCITY,
    SPRITESHEET,
    COLOR,
};

constexpr const char* AddonTypeStrings[] = {"None", "Base", "Velocity", "Spritesheet", "Color"};
constexpr const int AddonTypeStringsSize = sizeof(AddonTypeStrings) / sizeof(char*);

class ParticleUtils
{
  public:
    static void CreateEmptyParticleAddon(ParticleAddonType type, ParticleEmitter* emitter);
    static void CreateExistingComponent(const rapidjson::Value& initialState, ParticleEmitter* emitter);
};

#define ADDON_TYPES BaseAddon*, VelocityAddon*, SpritesheetAddon*, ColorAddon*

#define ADDON_NULLPTR nullptr, nullptr, nullptr, nullptr