#pragma once

#include "ParticleAddon.h"

#include "Geometry/AABB.h"
#include "Geometry/Circle.h"
#include "Geometry/OBB.h"
#include "Geometry/Sphere.h"
#include "Math/float3.h"
#include "Algorithm/Random/LCG.h"

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

    void UpdateShapesTransforms(const float4x4& globalTransform);
    void AssignPositionDirection(Particle& particle);

  private:
    void ManageShapeSwitch(ParticleAreaShape previousShape);

    void RenderCubeEditor();
    void RenderCircleEditor();
    void RenderSphereEditor();
    void RenderConeEditor();

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

    Circle circle;
    OBB cube;
    AABB basicCube;
    Sphere sphere;
    LCG areaRNG;
};
