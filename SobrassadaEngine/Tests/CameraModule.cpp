#include "CameraModule.h"

#include "Application.h"
#include "InputModule.h"
#include "WindowModule.h"

#include "DebugDraw/debugdraw.h"
#include "Math/Quat.h"
#include "MathGeoLib.h"
#include "SDL_scancode.h"
#include <functional>

CameraModule::CameraModule()
{
}

CameraModule::~CameraModule()
{
}

bool CameraModule::Init()
{
    camera.type              = FrustumType::PerspectiveFrustum;

    camera.pos               = float3(0, 1, 5);
    camera.front             = -float3::unitZ;
    camera.up                = float3::unitY;

    camera.nearPlaneDistance = 0.1f;
    camera.farPlaneDistance  = 100.f;

    camera.horizontalFov     = (float)HFOV * DEGTORAD;

    int width                = App->GetWindowModule()->GetWidth();
    int height               = App->GetWindowModule()->GetHeight();

    camera.verticalFov       = 2.0f * atanf(tanf(camera.horizontalFov * 0.5f) * ((float)height / (float)width));

    viewMatrix               = camera.ViewMatrix();
    projectionMatrix         = camera.ProjectionMatrix();

    std::function<void(void)> fPressed = std::bind(&CameraModule::EventTriggered, this);
    std::function<void(void)> oPressed = std::bind(&CameraModule::ToggleDetachedCamera, this);

    App->GetInputModule()->SubscribeToEvent(SDL_SCANCODE_F, fPressed);
    App->GetInputModule()->SubscribeToEvent(SDL_SCANCODE_O, oPressed);

    detachedCamera.type              = FrustumType::PerspectiveFrustum;

    detachedCamera.pos               = float3(0, 1, 5);
    detachedCamera.front             = -float3::unitZ;
    detachedCamera.up                = float3::unitY;
    detachedCamera.nearPlaneDistance = 0.1f;
    detachedCamera.farPlaneDistance  = 100.f;
    detachedCamera.horizontalFov     = (float)HFOV * DEGTORAD;
    camera.verticalFov               = 2.0f * atanf(tanf(camera.horizontalFov * 0.5f) * ((float)height / (float)width));

    detachedViewMatrix               = detachedCamera.ViewMatrix();
    detachedProjectionMatrix         = detachedCamera.ProjectionMatrix();

    return true;
}

update_status CameraModule::Update(float deltaTime)
{
    MoveCamera(deltaTime);
    frustumPlanes.UpdateFrustumPlanes(viewMatrix, projectionMatrix);

    return UPDATE_CONTINUE;
}

bool CameraModule::ShutDown()
{
    return true;
}

void CameraModule::SetAspectRatio(float newAspectRatio)
{
    camera.verticalFov         = 2.0f * atanf(tanf(camera.horizontalFov * 0.5f) * newAspectRatio);
    detachedCamera.verticalFov = 2.0f * atanf(tanf(camera.horizontalFov * 0.5f) * newAspectRatio);

    projectionMatrix           = camera.ProjectionMatrix();
    detachedProjectionMatrix   = detachedCamera.ProjectionMatrix();

    frustumPlanes.UpdateFrustumPlanes(viewMatrix, projectionMatrix);
}

void CameraModule::EventTriggered()
{
    GLOG("Event Trigered!!!!")
}

void CameraModule::ToggleDetachedCamera()
{
    isCameraDetached = !isCameraDetached;

    if (isCameraDetached)
    {
        detachedCamera.pos   = camera.pos;
        detachedCamera.front = camera.front;
        detachedCamera.up    = camera.up;
    }
}

void CameraModule::MoveCamera(float deltaTime)
{
    float finalCameraSpeed = cameraMoveSpeed * deltaTime;

    if (App->GetInputModule()->GetKey(SDL_SCANCODE_LSHIFT))
    {
        finalCameraSpeed *= 2;
    }

    if (App->GetInputModule()->GetMouseButtonDown(SDL_BUTTON_RIGHT))
    {
        // TRANSLATION
        if (App->GetInputModule()->GetKey(SDL_SCANCODE_W))
        {
            if (isCameraDetached) detachedCamera.pos += detachedCamera.front * finalCameraSpeed;
            else camera.pos += camera.front * finalCameraSpeed;
        }
        if (App->GetInputModule()->GetKey(SDL_SCANCODE_S))
        {
            if (isCameraDetached) detachedCamera.pos -= detachedCamera.front * finalCameraSpeed;
            else camera.pos -= camera.front * finalCameraSpeed;
        }

        if (App->GetInputModule()->GetKey(SDL_SCANCODE_A))
        {
            if (isCameraDetached) detachedCamera.pos -= detachedCamera.WorldRight() * finalCameraSpeed;
            else camera.pos -= camera.WorldRight() * finalCameraSpeed;
        }
        if (App->GetInputModule()->GetKey(SDL_SCANCODE_D))
        {
            if (isCameraDetached) detachedCamera.pos += detachedCamera.WorldRight() * finalCameraSpeed;
            else camera.pos += camera.WorldRight() * finalCameraSpeed;
        }

        if (App->GetInputModule()->GetKey(SDL_SCANCODE_E))
        {
            if (isCameraDetached) detachedCamera.pos += detachedCamera.up * finalCameraSpeed;
            else camera.pos += camera.up * finalCameraSpeed;
        }
        if (App->GetInputModule()->GetKey(SDL_SCANCODE_Q))
        {
            if (isCameraDetached) detachedCamera.pos -= detachedCamera.up * finalCameraSpeed;
            else camera.pos -= camera.up * finalCameraSpeed;
        }
    }

    if (App->GetInputModule()->GetMouseButtonDown(SDL_BUTTON_RIGHT))
    {
        // ZOOMING
        if (App->GetInputModule()->GetKey(SDL_SCANCODE_LALT))
        {
            float mouseY = App->GetInputModule()->GetMouseMotion().y;

            if (mouseY != 0)
            {
                if (isCameraDetached) detachedCamera.pos += detachedCamera.front * mouseY * finalCameraSpeed;
                else camera.pos += camera.front * mouseY * finalCameraSpeed;
            }
        }
        else
        {
            // ROTATION WITH MOUSE
            float mouseX             = App->GetInputModule()->GetMouseMotion().x;
            float mouseY             = App->GetInputModule()->GetMouseMotion().y;
            float deltaRotationAngle = cameraRotationAngle * deltaTime;

            if (mouseX != 0)
            {

                Quat yawRotation = Quat::RotateY(-mouseX * deltaRotationAngle);

                if (isCameraDetached)
                {
                    detachedCamera.front = yawRotation.Mul(detachedCamera.front).Normalized();
                    detachedCamera.up    = yawRotation.Mul(detachedCamera.up).Normalized();
                }
                else
                {
                    camera.front = yawRotation.Mul(camera.front).Normalized();
                    camera.up    = yawRotation.Mul(camera.up).Normalized();
                }
            }

            if (mouseY != 0)
            {

                if (isCameraDetached)
                {
                    Quat pitchRotation =
                        Quat::RotateAxisAngle(detachedCamera.WorldRight(), -mouseY * deltaRotationAngle);
                    if (detachedCamera.front.y < 0.9f || detachedCamera.front.y > -0.9f)
                    {
                        detachedCamera.front = pitchRotation.Mul(detachedCamera.front).Normalized();
                        detachedCamera.up    = pitchRotation.Mul(detachedCamera.up).Normalized();
                    }
                }
                else
                {
                    if (camera.front.y < 0.9f || camera.front.y > -0.9f)
                    {
                        Quat pitchRotation = Quat::RotateAxisAngle(camera.WorldRight(), -mouseY * deltaRotationAngle);
                        camera.front       = pitchRotation.Mul(camera.front).Normalized();
                        camera.up          = pitchRotation.Mul(camera.up).Normalized();
                    }
                }
            }
        }
    }

    viewMatrix         = camera.ViewMatrix();
    detachedViewMatrix = detachedCamera.ViewMatrix();
}