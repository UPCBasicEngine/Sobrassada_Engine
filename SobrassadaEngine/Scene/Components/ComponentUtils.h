#pragma once

#include "Globals.h"
#include "Delegate.h"

#include "Math/float4x4.h"
#include "rapidjson/document.h"
#include <cstdint>

class Component;
class GameObject;

enum ComponentType : int
{
    // Empty type
    COMPONENT_NONE = 0,
    // Standalone types
    COMPONENT_MESH,
    COMPONENT_POINT_LIGHT,
    COMPONENT_SPOT_LIGHT,
    COMPONENT_DIRECTIONAL_LIGHT,
    COMPONENT_CHARACTER_CONTROLLER,
    COMPONENT_CAMERA,
    COMPONENT_SCRIPT,
    COMPONENT_TRANSFORM_2D,
    COMPONENT_CANVAS,
    COMPONENT_LABEL,
    COMPONENT_CUBE_COLLIDER,
    COMPONENT_SPHERE_COLLIDER,
    COMPONENT_CAPSULE_COLLIDER,
    COMPONENT_ANIMATION,
    COMPONENT_AIAGENT,
    COMPONENT_IMAGE,
    COMPONENT_BUTTON,
    COMPONENT_AUDIO_SOURCE,
    COMPONENT_AUDIO_LISTENER,
};

enum class ColliderType : uint8_t
{
    STATIC = 0,
    DYNAMIC,
    KINEMATIC,
    TRIGGER

};

constexpr const char* ColliderTypeStrings[] = {"Static", "Dynamic", "Kinematic", "Trigger"};

// LAYERS IN THE PHYSICS WORLD ARE REPRESENTED AS AN INT, SO 32 LAYERS ARE THE MAXIMUM ALLOWED WITH CURRENT
// IMPLEMENTATION
enum class ColliderLayer : uint8_t
{
    WORLD_OBJECTS = 0,
    TRIGGERS,
    ENEMY,
    PLAYER,
    PLAYER_PROJECTILE,
    ENEMY_PROJECTILE
};

constexpr const char* ColliderLayerStrings[] = {"World Objects", "Triggers",          "Enemies",
                                                "Player",        "Player projectile", "Enemy projectile"};

typedef Delegate<void, GameObject*, float3> CollisionDelegate;

class ComponentUtils
{
  public:
    static void CreateEmptyComponent(ComponentType type, UID uid, GameObject* parent);

    static void CreateExistingComponent(const rapidjson::Value& initialState, GameObject* parent);
};

#define COMPONENTS                                                                                                     \
    MeshComponent*, PointLightComponent*, SpotLightComponent*, DirectionalLightComponent*,                             \
        CharacterControllerComponent*, Transform2DComponent*, CanvasComponent*,         \
        UILabelComponent*, CameraComponent*, ScriptComponent*, CubeColliderComponent*, SphereColliderComponent*,       \
        CapsuleColliderComponent*, AnimationComponent*, AIAgentComponent*, ImageComponent*, ButtonComponent*,          \
        AudioSourceComponent*, AudioListenerComponent*

#define COMPONENTS_NULLPTR                                                                                             \
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,        \
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr