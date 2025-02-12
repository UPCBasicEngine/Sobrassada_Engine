#include "CameraModule.h"

#include "Application.h"
#include "WindowModule.h"
#include "InputModule.h"
#include "glew.h"

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

	matrices.viewMatrix = camera.ViewMatrix();
	matrices.projectionMatrix = camera.ProjectionMatrix();

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
	return UPDATE_CONTINUE;
}

bool CameraModule::ShutDown()
{
    glDeleteBuffers(1, &ubo);
	return true;
}

void CameraModule::SetAspectRatio(float newAspectRatio)
{
	camera.verticalFov = 2.0f * atanf(tanf(camera.horizontalFov * 0.5f) * newAspectRatio);
	matrices.viewMatrix = camera.ViewMatrix();
	matrices.projectionMatrix = camera.ProjectionMatrix();
}

void CameraModule::EventTriggered()
{
	GLOG("Event Trigered!!!!")
}