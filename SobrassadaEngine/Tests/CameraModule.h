#pragma once

#include "FrustumPlanes.h"
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

    const float4x4& GetProjectionMatrix() { return isCameraDetached ? detachedProjectionMatrix : projectionMatrix; }
    const float4x4& GetViewMatrix() { return isCameraDetached ? detachedViewMatrix : viewMatrix; }

    const float4x4& GetFrustumViewMatrix() { return viewMatrix; }
    const float4x4& GetFrustumProjectionMatrix() { return projectionMatrix; }
    const FrustumPlanes& GetFrustrumPlanes() const { return frustumPlanes; }

    bool IsCameraDetached() const { return isCameraDetached; }

    void SetAspectRatio(float newAspectRatio);

  private:
    void EventTriggered();
    void ToggleDetachedCamera();
    void MoveCamera();

  private:
    Frustum camera;
    Frustum detachedCamera;

    AABB frustumAABB;

    float4x4 viewMatrix;
    float4x4 projectionMatrix;

    float4x4 detachedViewMatrix;
    float4x4 detachedProjectionMatrix;

    FrustumPlanes frustumPlanes;

    float speedBase       = 0.01f;
    float movementSpeed   = 0.01f;
    bool isCameraDetached = false;
};
