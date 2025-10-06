#pragma once

#include "ParticleAddon.h"

#include "Geometry/AABB.h"
#include "Geometry/Circle.h"
#include "Geometry/OBB.h"
#include "Geometry/Sphere.h"
#include "Math/float3.h"
#include "Math/float4x4.h"

struct Particle;

class AreaAddon : public ParticleAddon
{
  public:
    AreaAddon(ParticleEmitter* owner);
    AreaAddon(const rapidjson::Value& initialState, ParticleEmitter* owner);
    ~AreaAddon() override;

    void Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const override;

    void Init(EmitterInstance* emitterInstance);
    void Update(float deltaTime, EmitterInstance* emitterInstance) override;
    void RenderEditorInspector() override;
    void RenderDebug(GameObject* parent) override;
    void Duplicate(ParticleAddon* reference) override;

    void UpdateShapesTransforms(const float4x4& globalTransform);
    void AssignPositionDirection(Particle& particle);

    void AssignMaxValues(ParticleValues& particleValue) override;

  private:
    void ManageShapeSwitch(ParticleAreaShape previousShape);

    void RenderCubeEditor(bool& anyChange);
    void RenderCircleEditor(bool& anyChange);
    void RenderSphereEditor(bool& anyChange);
    void RenderConeEditor(bool& anyChange);

    void RecalculateConeTopRadius();

  private:
    ParticleAreaShape currentShape = ParticleAreaShape::NONE;
    ParticleAreaSpawn currentSpawn = ParticleAreaSpawn::NONE;

    // Circle, Shpere radius, cone radius
    float baseRadius               = 1.f;

    // Cone top radius
    float topRadius                = 1.f;

    // Cone angle
    float coneAngle                = 35.f;
    float coneLength               = 1.f;

    float3 cubeSize                = float3::one;
    float4x4 lastGlobalTransform   = float4x4::identity;

    Circle circle;
    OBB cube;
    AABB basicCube;
    Sphere sphere;
};
