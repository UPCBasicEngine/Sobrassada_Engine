#pragma once

#include "CameraModule.h"
#include "Component.h"

#include "Geometry/Frustum.h"
#include "Math/float4x4.h"
#include "rapidjson/document.h"

class Framebuffer;

class CameraComponent : public Component
{
  public:
    CameraComponent(UID uid, GameObject* parent);
    CameraComponent(const rapidjson::Value& initialState, GameObject* parent);
    ~CameraComponent() override;

    void Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const override;
    void Clone(const Component* other) override;

    void Update(float deltaTime) override;
    void Render(float deltaTime) override;
    void RenderDebug(float deltaTime) override;
    void RenderEditorInspector() override;

    void ChangeToPerspective();
    void ChangeToOrtographic();

    void RenderCameraPreview(float deltaTime);

    void SOBRASADA_API_ENGINE Translate(const float3& direction);
    void SOBRASADA_API_ENGINE Rotate(float yaw, float pitch);
    const LineSegment CastCameraRay();
    const SOBRASADA_API_ENGINE float3 ScreenPointToXZ(const float y);

    const FrustumPlanes& GetFrustrumPlanes() const { return frustumPlanes; }
    const float3& GetCameraPosition() const { return camera.pos; }
    const float3& GetCameraFront() const { return camera.front; }
    float3 GetCameraRight() const { return camera.WorldRight(); }
    const float3& GetCameraUp() const { return camera.up; }
    unsigned int GetUbo() const { return ubo; }
    const float4x4 GetProjectionMatrix() { return camera.ProjectionMatrix(); }
    const float4x4 GetViewMatrix() { return camera.ViewMatrix(); }
    const int GetFrustumType() { return (camera.type == OrthographicFrustum) ? 1 : 0; }
    Framebuffer* GetFramebuffer() { return previewFramebuffer; }

    void SetAspectRatio(float newAspectRatio);
    void SetCameraPosition(const float3& position) { camera.pos = position; }
    void SetCameraFront(const float3& front) { camera.front = front; }
    void SetCameraUp(const float3& up) { camera.up = up; }
    void SetFreeCamera(const bool freecamera) { freeCamera = freecamera; }

  private:
    Frustum camera;
    FrustumPlanes frustumPlanes;
    CameraMatrices matrices;
    unsigned int ubo  = 0;

    bool drawGizmos   = true;
    bool isMainCamera = false;

    float horizontalFov;
    float verticalFov;
    float perspectiveNearPlane = 0.1f;
    float perspectiveFarPlane  = 50.0f;

    float orthographicWidth;
    float orthographicHeight;
    float ortographicNearPlane      = 25.10f;
    float ortographicFarPlane       = 50.0f;

    bool firstTime                  = true;

    bool previewEnabled             = false;
    bool seePreview                 = true;

    Framebuffer* previewFramebuffer = nullptr;
    int previewWidth                = 256;
    int previewHeight               = 256;

    bool autorendering              = false;
    bool firstFrame                 = false;
    bool freeCamera                 = false;
    float currentPitchAngle   = 0.f;
};
