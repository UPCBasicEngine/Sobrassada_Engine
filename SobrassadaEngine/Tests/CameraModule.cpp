#include "CameraModule.h"

#include "Application.h"
#include "WindowModule.h"
#include "InputModule.h"

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
	return UPDATE_CONTINUE;
}

bool CameraModule::ShutDown()
{
	return true;
}

void CameraModule::SetAspectRatio(float newAspectRatio)
{
	camera.verticalFov = 2.0f * atanf(tanf(camera.horizontalFov * 0.5f) * newAspectRatio);
	viewMatrix = camera.ViewMatrix();
	projectionMatrix = camera.ProjectionMatrix();
}

void CameraModule::EventTriggered()
{
	GLOG("Event Trigered!!!!")
}
