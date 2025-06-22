#pragma once

#include "FrustumPlanes.h"
#include "Module.h"

#include "Geometry/AABB.h"
#include "Geometry/Frustum.h"
#include "Geometry/LineSegment.h"
#include "Math/float4x4.h"
#include "rapidjson/document.h"

constexpr float cameraRotationAngle  = 135.f / RAD_DEGREE_CONV;
constexpr float maximumPositivePitch = 89.f / RAD_DEGREE_CONV;
constexpr float maximumNegativePitch = -89.f / RAD_DEGREE_CONV;

struct CameraMatrices
{
    float4x4 projectionMatrix;
    float4x4 viewMatrix;
};

class CameraModule : public Module
{
  public:
    CameraModule();
    ~CameraModule() override;

    bool Init() override;
    update_status Update(float deltaTime) override;
    bool ShutDown() override;

    bool IsCameraDetached() const { return isCameraDetached; }
    void UpdateUBO();
    const LineSegment& CastCameraRay();
    void SaveCameraPosition(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator);
    void LoadCameraPosition(const rapidjson::Value* initialState);

    const float4x4& GetProjectionMatrix() { return isCameraDetached ? detachedProjectionMatrix : projectionMatrix; }
    const float4x4& GetViewMatrix() { return isCameraDetached ? detachedViewMatrix : viewMatrix; }
    const float4x4& GetFrustumViewMatrix() { return viewMatrix; }
    const float4x4& GetFrustumProjectionMatrix() { return projectionMatrix; }
    const FrustumPlanes& GetFrustrumPlanes() const { return frustumPlanes; }
    void GetFrustumCorners(float3* corners) const { return camera.GetCornerPoints(corners); }
    const float3& GetCameraPosition() const { return isCameraDetached ? detachedCamera.pos : camera.pos; }
    const Frustum& GetCamera() const { return camera; }

    float GetNearPlaneDistance() const
    {
        return isCameraDetached ? detachedCamera.nearPlaneDistance : camera.nearPlaneDistance;
    }

    float GetFarPlaneDistance() const
    {
        return isCameraDetached ? detachedCamera.farPlaneDistance : camera.farPlaneDistance;
    }

    unsigned int GetUbo() const { return ubo; }
    const LineSegment& GetLastCastedRay() const { return lastCastedRay; }
    bool GetOrbiting() const { return orbiting; }

    void SetAspectRatio(float newAspectRatio);
    void SetNear(float nearPlane)
    {
        camera.nearPlaneDistance = nearPlane;
        UpdateUBO();
    }
    void SetFar(float farPlane)
    {
        camera.farPlaneDistance = farPlane;
        UpdateUBO();
    }

  private:
    void Controls(float deltaTime);
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
    float rotateSensitivity   = DEFAULT_CAMERA_ROTATE_SENSITIVITY;
    float dragSensitivity     = DEFAULT_CAMERA_DRAG_SENSITIVITY;
    float wheelSensitivity    = DEFAULT_CAMERA_WHEEL_SENSITIVITY;
    float zoomSensitivity     = DEFAULT_CAMERA_ZOOM_SENSITIVITY;

    float currentPitchAngle   = 0.f;

    bool isCameraDetached     = false;
    bool orbiting             = false;

    CameraMatrices matrices;

    unsigned int ubo;

    LineSegment lastCastedRay;
};
