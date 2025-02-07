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

	std::function<void(void)> rotateLeft = std::bind(&CameraModule::RotateLeft, this);
	std::function<void(void)> rotateRight = std::bind(&CameraModule::RotateRight, this);
	std::function<void(void)> rotateUp = std::bind(&CameraModule::RotateUp, this);
	std::function<void(void)> rotateDown = std::bind(&CameraModule::RotateDown, this);

	std::function<void(void)> moveForward = std::bind(&CameraModule::MoveForward, this);
	std::function<void(void)> moveBackward = std::bind(&CameraModule::MoveBackward, this);
	std::function<void(void)> moveLeft = std::bind(&CameraModule::MoveLeft, this);
	std::function<void(void)> moveRight = std::bind(&CameraModule::MoveRight, this);
	std::function<void(void)> moveUp = std::bind(&CameraModule::MoveUp, this);
	std::function<void(void)> moveDown = std::bind(&CameraModule::MoveDown, this);

	App->GetInputModule()->SubscribeToEvent(SDL_SCANCODE_Q, rotateLeft);
	App->GetInputModule()->SubscribeToEvent(SDL_SCANCODE_E, rotateRight);
	App->GetInputModule()->SubscribeToEvent(SDL_SCANCODE_R, rotateUp);
	App->GetInputModule()->SubscribeToEvent(SDL_SCANCODE_F, rotateDown);

	App->GetInputModule()->SubscribeToEvent(SDL_SCANCODE_W, moveForward);
	App->GetInputModule()->SubscribeToEvent(SDL_SCANCODE_S, moveBackward);
	App->GetInputModule()->SubscribeToEvent(SDL_SCANCODE_A, moveLeft);
	App->GetInputModule()->SubscribeToEvent(SDL_SCANCODE_D, moveRight);
	App->GetInputModule()->SubscribeToEvent(SDL_SCANCODE_Z, moveUp);
	App->GetInputModule()->SubscribeToEvent(SDL_SCANCODE_X, moveDown);

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

void CameraModule::MoveForward()
{
    float speed = 1.0f; // Velocidad de movimiento
    camera.pos += camera.front * speed;
	viewMatrix = camera.ViewMatrix();
}

void CameraModule::MoveBackward()
{
    float speed = 1.0f;
    camera.pos -= camera.front * speed;
	viewMatrix = camera.ViewMatrix();
}

void CameraModule::MoveLeft()
{
    float speed = 1.0f;
	float3 right = camera.up.Cross(camera.front).Normalized();
    camera.pos += right * speed;
	viewMatrix = camera.ViewMatrix();
}

void CameraModule::MoveRight()
{
    float speed = 1.0f;
    float3 right = camera.up.Cross(camera.front).Normalized();
    camera.pos -= right * speed;
	viewMatrix = camera.ViewMatrix();
}

void CameraModule::RotateLeft()
{
    float angle = 10.0f * DEGTORAD; // Convierte grados a radianes
    camera.front = Quat::RotateY(angle).Transform(camera.front);
	viewMatrix = camera.ViewMatrix();
}

void CameraModule::RotateRight()
{
    float angle = -10.0f * DEGTORAD; // Negativo para rotar a la derecha
    camera.front = Quat::RotateY(angle).Transform(camera.front);
	viewMatrix = camera.ViewMatrix();
}

void CameraModule::RotateUp()
{
	float3 right = camera.up.Cross(camera.front).Normalized();  // Calculate right vector for local rotation
    Quat pitch_rotation = Quat::RotateAxisAngle(right, -5 * DEGTORAD);
	camera.front = pitch_rotation.Transform(camera.front);
    camera.up = pitch_rotation.Transform(camera.up);

	viewMatrix = camera.ViewMatrix();
}

void CameraModule::RotateDown()
{
    float3 right = camera.up.Cross(camera.front).Normalized();  // Calculate right vector for local rotation
    Quat pitch_rotation = Quat::RotateAxisAngle(right, 5 * DEGTORAD);
	camera.front = pitch_rotation.Transform(camera.front);
    camera.up = pitch_rotation.Transform(camera.up);
	viewMatrix = camera.ViewMatrix();
}

void CameraModule::MoveUp()
{
    camera.pos.y += 0.1f;
	viewMatrix = camera.ViewMatrix();
}

void CameraModule::MoveDown()
{

    camera.pos.y -= 0.1f;
	viewMatrix = camera.ViewMatrix();
}