#include "CameraModule.h"

#include "Application.h"
#include "InputModule.h"
#include "glew.h"
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

    App->GetInputModule()->SubscribeToEvent(SDL_SCANCODE_F, fPressed);


	glGenBuffers(1, &ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, ubo);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(CameraMatrices), nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);


	return true;
}

void CameraModule::UpdateUBO() 
{
	glBindBuffer(GL_UNIFORM_BUFFER, ubo);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(CameraMatrices), &matrices);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

update_status CameraModule::Update(float deltaTime)
{
    MoveCamera();
    frustumPlanes.UpdateFrustumPlanes(viewMatrix, projectionMatrix);

    return UPDATE_CONTINUE;
}

bool CameraModule::ShutDown()
{
    return true;
}

void CameraModule::SetAspectRatio(float newAspectRatio)
{
    camera.verticalFov = 2.0f * atanf(tanf(camera.horizontalFov * 0.5f) * newAspectRatio);
    projectionMatrix   = camera.ProjectionMatrix();

    frustumPlanes.UpdateFrustumPlanes(viewMatrix, projectionMatrix);
}

void CameraModule::EventTriggered()
{
    GLOG("Event Trigered!!!!")
}


void CameraModule::MoveCamera()
{
    movementSpeed = speedBase;

    if (App->GetInputModule()->GetKey(SDL_SCANCODE_LSHIFT))
    {
        movementSpeed = speedBase * 3;
    }

    if (App->GetInputModule()->GetMouseButtonDown(SDL_BUTTON_RIGHT))
    {
        // TRANSLATION
        if (App->GetInputModule()->GetKey(SDL_SCANCODE_W))
        {
            camera.pos += camera.front * movementSpeed;
        }
        if (App->GetInputModule()->GetKey(SDL_SCANCODE_S))
        {
            camera.pos -= camera.front * movementSpeed;
        }

        if (App->GetInputModule()->GetKey(SDL_SCANCODE_A))
        {
            camera.pos -= camera.WorldRight() * movementSpeed;
        }
        if (App->GetInputModule()->GetKey(SDL_SCANCODE_D))
        {
            camera.pos += camera.WorldRight() * movementSpeed;
        }

        if (App->GetInputModule()->GetKey(SDL_SCANCODE_E))
        {
            camera.pos += camera.up * movementSpeed;
        }
        if (App->GetInputModule()->GetKey(SDL_SCANCODE_Q))
        {
            camera.pos -= camera.up * movementSpeed;
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
                camera.pos += camera.front * mouseY * movementSpeed;
            }
        }
        else
        {
            // ROTATION WITH MOUSE
            float mouseX = App->GetInputModule()->GetMouseMotion().x;
            float mouseY = App->GetInputModule()->GetMouseMotion().y;

            if (mouseX != 0)
            {
                Quat yawRotation = Quat::RotateY(-mouseX * movementSpeed);
                camera.front     = yawRotation.Mul(camera.front).Normalized();
                camera.up        = yawRotation.Mul(camera.up).Normalized();
            }

            if (mouseY != 0)
            {
                if (camera.front.y < 0.9f || camera.front.y > -0.9f)
                {
                    Quat pitchRotation = Quat::RotateAxisAngle(camera.WorldRight(), -mouseY * movementSpeed);
                    camera.front       = pitchRotation.Mul(camera.front).Normalized();
                    camera.up          = pitchRotation.Mul(camera.up).Normalized();
                }
            }
        }
    }

    viewMatrix = camera.ViewMatrix();
}