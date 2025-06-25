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
    AREA,
};

constexpr const char* AddonTypeStrings[]          = {"None", "Base", "Velocity", "Spritesheet", "Color", "Area"};
constexpr const int AddonTypeStringsSize          = sizeof(AddonTypeStrings) / sizeof(char*);

enum class ParticleInterpolationType : int
{
    FIXED_VALUES = 0,
    BEZIER_SINGLE,
    CURVE_EDITOR,
};

constexpr const char* InterpolationAddonStrings[] = {"Fixed values", "Bezier single curve", "Curve editor"};
constexpr int InterpolationAddonStringsSize       = sizeof(InterpolationAddonStrings) / sizeof(char*);

enum class ParticleAreaShape : int
{
    NONE = -1,
    CUBE,
    CIRCLE,
    SPHERE,
    CONE,
};

constexpr const char* AreaAddonStrings[] = {"Cube", "Circle", "Sphere", "Cone"};
constexpr int AreaAddonStringsSize       = sizeof(AreaAddonStrings) / sizeof(char*);

enum class ParticleAreaSpawn : int
{
    NONE = -1,
    SURFACE,
    VOLUME,
};

constexpr const char* AreaAddonSpawnStrings[] = {"Surface", "Volume"};
constexpr int AreaAddonSpawnStringsSize       = sizeof(AreaAddonSpawnStrings) / sizeof(char*);

enum class EmitterBlendingMode : int
{
    ALPHA = 0,
    ALPHA_ADDITIVE,
    ADDITIVE,
};

constexpr const char* EmitterBlendingModeStrings[] = {"Alpha", "Alpha additive", "Additive"};
constexpr int EmitterBlendingModeStringsSize       = sizeof(EmitterBlendingModeStrings) / sizeof(char*);

constexpr int MaxCurveEditorPoints                 = 10;

class ParticleUtils
{
  public:
    static void CreateEmptyParticleAddon(ParticleAddonType type, ParticleEmitter* emitter);
    static void CreateExistingComponent(const rapidjson::Value& initialState, ParticleEmitter* emitter);
};

#define ADDON_TYPES BaseAddon*, VelocityAddon*, SpritesheetAddon*, ColorAddon*, AreaAddon*

#define ADDON_NULLPTR nullptr, nullptr, nullptr, nullptr, nullptr