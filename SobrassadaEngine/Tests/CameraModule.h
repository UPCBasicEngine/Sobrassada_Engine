#pragma once

#include "FrustumPlanes.h"
#include "Module.h"

#include "Geometry/AABB.h"
#include "Geometry/Frustum.h"
#include "Math/Quat.h"
#include "Math/float4x4.h"

constexpr float DEGTORAD             = PI / 180.f;
constexpr float cameraRotationAngle  = 135.f * DEGTORAD;
constexpr float maximumPositivePitch = 89.f * DEGTORAD;
constexpr float maximumNegativePitch = -89.f * DEGTORAD;

struct CameraMatrices
{
	float4x4 projectionMatrix;
	float4x4 viewMatrix;
};

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
    const float3& GetCameraPosition() const { return isCameraDetached ? detachedCamera.pos : camera.pos; }

    unsigned int GetUbo() const { return ubo; }
    void UpdateUBO();

    bool IsCameraDetached() const { return isCameraDetached; }

    void SetAspectRatio(float newAspectRatio);

  private:
    void TriggerFocusCamera();
    void ToggleDetachedCamera();
    void RotateCamera(float yaw, float pitch);
    void FocusCamera();

  private:
    Frustum camera;
    Frustum detachedCamera;

    float4x4 viewMatrix;
    float4x4 projectionMatrix;

    float4x4 detachedViewMatrix;
    float4x4 detachedProjectionMatrix;

    FrustumPlanes frustumPlanes;

    float movementScaleFactor = DEFAULT_CAMERA_MOVEMENT_SCALE_FACTOR;
    float cameraMoveSpeed     = DEFAULT_CAMERA_MOVEMENT_SPEED;
    float mouseSensitivity    = DEFAULT_CAMERA_MOUSE_SENSITIVITY;
    float zoomSensitivity     = DEFAULT_CAMERA_ZOOM_SENSITIVITY;
    float currentPitchAngle   = 0.f;

    bool isCameraDetached     = false;

    CameraMatrices matrices;

    unsigned int ubo;
};
