#pragma once

#include "Module.h"

#include "Math/float4x4.h"
#include "Geometry/Frustum.h"
#include "Math/Quat.h"

constexpr float DEGTORAD = PI / 180.f;
constexpr float maximumPositivePitch = 89.f * DEGTORAD;
constexpr float maximumNegativePitch = -89.f * DEGTORAD;
constexpr float cameraRotationAngle = 135.f * DEGTORAD;

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

	const float4x4& GetProjectionMatrix() { return matrices.projectionMatrix; }
	const float4x4& GetViewMatrix() { return matrices.viewMatrix; }
	const unsigned int GetUbo() { return ubo; }
	void UpdateUBO();

	void SetAspectRatio(float newAspectRatio);

	void EventTriggered();

	const float3 getPosition() { return camera.pos; }

private:
	Frustum camera;
	CameraMatrices matrices;

	unsigned int ubo;
};

