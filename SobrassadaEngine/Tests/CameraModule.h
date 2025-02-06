#pragma once

#include "Module.h"

#include "Math/float4x4.h"
#include "Geometry/Frustum.h"

constexpr float DEGTORAD = PI / 180.f;
constexpr float maximumPositivePitch = 89.f * DEGTORAD;
constexpr float maximumNegativePitch = -89.f * DEGTORAD;
constexpr float cameraRotationAngle = 135.f * DEGTORAD;

class CameraModule : public Module
{
public:
	CameraModule();
	~CameraModule();

	bool Init() override;
	update_status Update(float deltaTime) override;
	bool ShutDown() override;

	const float4x4& GetProjectionMatrix() { return projectionMatrix; }
	const float4x4& GetViewMatrix() { return viewMatrix; }
    void MoveCamera();

	void SetAspectRatio(float newAspectRatio);

	void EventTriggered();

private:
	Frustum camera;

	float4x4 viewMatrix;
	float4x4 projectionMatrix;

	float speedBase;
    float movementSpeed = 2.0f;
};

