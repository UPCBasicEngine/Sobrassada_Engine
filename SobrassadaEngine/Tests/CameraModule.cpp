#include "CameraModule.h"

#include "Application.h"
#include "WindowModule.h"
#include "InputModule.h"

#include "SDL_scancode.h"
#include "Math/Quat.h"
#include "MathGeoLib.h"
#include "DebugDraw/debugdraw.h"
#include <functional>

CameraModule::CameraModule() { frustumPlanes.assign(6, float4(0,0,0,0)); }

CameraModule::~CameraModule()
{
}

bool CameraModule::Init()
{
	camera.type = FrustumType::PerspectiveFrustum;

	camera.pos = float3(0, 1, 5);
	camera.front = -float3::unitZ;
	camera.up = float3::unitY;

	camera.nearPlaneDistance = 0.1f;
	camera.farPlaneDistance = 100.f;

	camera.horizontalFov = (float)HFOV * DEGTORAD;

	int width = App->GetWindowModule()->GetWidth();
	int height = App->GetWindowModule()->GetHeight();

	camera.verticalFov = 2.0f * atanf(tanf(camera.horizontalFov * 0.5f) * ((float)height / (float)width));

	viewMatrix = camera.ViewMatrix();
	projectionMatrix = camera.ProjectionMatrix();

	std::function<void(void)> fPressed = std::bind(&CameraModule::EventTriggered, this);

	App->GetInputModule()->SubscribeToEvent(SDL_SCANCODE_F, fPressed);

	return true;
}

update_status CameraModule::Update(float deltaTime)
{
    MoveCamera();
    GetFrustumAABB();
	return UPDATE_CONTINUE;
}

bool CameraModule::ShutDown()
{
	return true;
}

void CameraModule::SetAspectRatio(float newAspectRatio)
{
	camera.verticalFov = 2.0f * atanf(tanf(camera.horizontalFov * 0.5f) * newAspectRatio);
	projectionMatrix = camera.ProjectionMatrix();

    ExtractFrustumPlanes();
}

void CameraModule::EventTriggered()
{
	GLOG("Event Trigered!!!!") }

void CameraModule::ExtractFrustumPlanes() {

    float4x4 vpMatrix = projectionMatrix * viewMatrix;

    for (int i = 0; i < 6; ++i)
    {

        // Plano Izquierda
        if (i == 0)
        {
            frustumPlanes[i].x = vpMatrix[3][0] + vpMatrix[0][0];
            frustumPlanes[i].y = vpMatrix[3][1] + vpMatrix[0][1];
            frustumPlanes[i].z = vpMatrix[3][2] + vpMatrix[0][2];
            frustumPlanes[i].w = vpMatrix[3][3] + vpMatrix[0][3];
        }
        // Plano Derecha
        else if (i == 1)
        {
            frustumPlanes[i].x = vpMatrix[3][0] - vpMatrix[0][0];
            frustumPlanes[i].y = vpMatrix[3][1] - vpMatrix[0][1];
            frustumPlanes[i].z = vpMatrix[3][2] - vpMatrix[0][2];
            frustumPlanes[i].w = vpMatrix[3][3] - vpMatrix[0][3];
        }
        // Plano abajo
        else if (i == 2)
        {
            frustumPlanes[i].x = vpMatrix[3][0] + vpMatrix[1][0];
            frustumPlanes[i].y = vpMatrix[3][1] + vpMatrix[1][1];
            frustumPlanes[i].z = vpMatrix[3][2] + vpMatrix[1][2];
            frustumPlanes[i].w = vpMatrix[3][3] + vpMatrix[1][3];
        }
        // Plano arriba
        else if (i == 3)
        {
            frustumPlanes[i].x = vpMatrix[3][0] - vpMatrix[1][0];
            frustumPlanes[i].y = vpMatrix[3][1] - vpMatrix[1][1];
            frustumPlanes[i].z = vpMatrix[3][2] - vpMatrix[1][2];
            frustumPlanes[i].w = vpMatrix[3][3] - vpMatrix[1][3];
        }
        // Plano Near
        else if (i == 4)
        {
            frustumPlanes[i].x = vpMatrix[3][0] + vpMatrix[2][0];
            frustumPlanes[i].y = vpMatrix[3][1] + vpMatrix[2][1];
            frustumPlanes[i].z = vpMatrix[3][2] + vpMatrix[2][2];
            frustumPlanes[i].w = vpMatrix[3][3] + vpMatrix[2][3];
        }
        // Plano Far
        else if (i == 5)
        {
            frustumPlanes[i].x = vpMatrix[3][0] - vpMatrix[2][0];
            frustumPlanes[i].y = vpMatrix[3][1] - vpMatrix[2][1];
            frustumPlanes[i].z = vpMatrix[3][2] - vpMatrix[2][2];
            frustumPlanes[i].w = vpMatrix[3][3] - vpMatrix[2][3];
        }

        frustumPlanes[i].Normalize4();
    }
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
    ExtractFrustumPlanes();
}

const AABB& CameraModule::GetFrustumAABB()
{
    float3 corners[8];
    camera.GetCornerPoints(corners);
    
    frustumAABB.Enclose(corners, 8); 

    return frustumAABB;
}


