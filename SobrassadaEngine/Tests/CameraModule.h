#pragma once

#include "Module.h"

#include "Geometry/AABB.h"
#include "Geometry/Frustum.h"
#include "Math/float4x4.h"

constexpr float DEGTORAD             = PI / 180.f;
constexpr float maximumPositivePitch = 89.f * DEGTORAD;
constexpr float maximumNegativePitch = -89.f * DEGTORAD;
constexpr float cameraRotationAngle  = 135.f * DEGTORAD;

class CameraModule : public Module
{
  public:
    CameraModule();
    ~CameraModule();

    bool Init() override;
    update_status Update(float deltaTime) override;
    bool ShutDown() override;

    const float4x4 &GetProjectionMatrix() { return projectionMatrix; }
    const float4x4 &GetViewMatrix() { return viewMatrix; }
    const std::vector<float4> &GetFrustrumPlanes() const { return frustumPlanes; }
    void MoveCamera();
    const AABB &GetFrustumAABB();

    void SetAspectRatio(float newAspectRatio);

    void EventTriggered();

  private:
    void ExtractFrustumPlanes();

  private:
    Frustum camera;
    AABB frustumAABB;

    float4x4 viewMatrix;
    float4x4 projectionMatrix;
    std::vector<float4> frustumPlanes;

    float speedBase     = 0.01f;
    float movementSpeed = 0.01f;
};
