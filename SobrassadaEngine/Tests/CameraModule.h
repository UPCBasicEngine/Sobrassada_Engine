#pragma once

#include "FrustumPlanes.h"
#include "Module.h"

#include "Geometry/AABB.h"
#include "Geometry/Frustum.h"
#include "Math/float4x4.h"

constexpr float DEGTORAD            = PI / 180.f;
constexpr float cameraRotationAngle = 135.f * DEGTORAD;
constexpr float maximumPositivePitch = 89.f * DEGTORAD;
constexpr float maximumNegativePitch = -89.f * DEGTORAD;

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
    void TriggerFocusCamera();
    void ToggleDetachedCamera();
    void MoveCamera(float deltaTime);
    void FocusCamera();
    const float4x4& LookAt(const float3& cameraPosition, const float3& targetPosition, const float3& upVector) const;

  private:
    Frustum camera;
    Frustum detachedCamera;

    float4x4 viewMatrix;
    float4x4 projectionMatrix;

    float4x4 detachedViewMatrix;
    float4x4 detachedProjectionMatrix;

    FrustumPlanes frustumPlanes;

    float movementScaleFactor = DEFAULT_CAMERA_MOVEMENT_SCALE_FACTOR;
    float cameraMoveSpeed     = DEFAULT_CAMERA_MOVMENT_SPEED;
    float mouseSensitivity    = DEFAULT_CAMERA_MOUSE_SENSITIVITY;
    float zoomSensitivity     = DEFAUL_CAMERA_ZOOM_SENSITIVITY;
    float currentPitchAngle   = 0.f;

    bool isCameraDetached     = false;
};
