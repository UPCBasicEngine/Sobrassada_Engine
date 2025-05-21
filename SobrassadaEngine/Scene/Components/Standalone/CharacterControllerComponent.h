#pragma once

#include "Component.h"

#include "Math/float3.h"

class dtNavMeshQuery;

using dtPolyRef = unsigned int;

class SOBRASADA_API_ENGINE CharacterControllerComponent : public Component
{

  public:
    CharacterControllerComponent(UID uid, GameObject* parent);
    CharacterControllerComponent(const rapidjson::Value& initialState, GameObject* parent);

    ~CharacterControllerComponent() override;

    void Update(float deltaTime) override;
    void Render(float deltaTime) override;
    void RenderDebug(float deltaTime) override;
    void RenderEditorInspector() override;
    void Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const override;
    void Clone(const Component* other) override;

    void AdjustHeightToNavMesh(float3& currentPos);
    void Move(float deltaTime);
    void LookAtMovement(const float3& moveDir, float deltaTime);
    void Rotate(float rotationDirection, float deltaTime);
    void SetDirection(float3& direction);
    void LookAt(const float3& direction);

    const float3& GetTargetDirection() const { return targetDirection; }
    const float3& GetFrontDirection() const { return rotateDirection; }
    const float3& GetLastPosition() const { return lastPosition; }
    const float& GetSpeed() const { return currentSpeed; }
    const float& GetMaxSpeed() const { return maxSpeed; }

    void SetTargetDirection(float3 newTargetDirection) { targetDirection = newTargetDirection; }
    void SetMaxSpeed(float newSpeed) { maxSpeed = newSpeed; }
    void SetInputDown(bool input) { inputDown = input; }
    void EnableMovement(bool enable) { movementEnabled = enable; }
    void StartDash();
    void EndDash() { isDashing = false; }

  private:
    void Dash(float deltaTime);

  private:
    float3 targetDirection       = float3::zero;
    float3 lastPosition          = float3::zero;

    float maxSpeed               = 10.0f;
    float maxAngularSpeed        = 0.0f;
    float acceleration           = 10.0f;
    float currentSpeed           = 0.0f;

    bool isRadians               = false;

    dtNavMeshQuery* navMeshQuery = nullptr;
    dtPolyRef currentPolyRef     = 0;

    float gravity                = -9.81f;
    float verticalSpeed          = 0.0f;
    float maxFallSpeed           = -20.0f;

    bool inputDown               = true;
    bool isRotating              = false;
    float3 targetLookDirection   = float3::zero;

    float3 rotateDirection       = float3::unitZ;
    bool movementEnabled         = true;

    bool isDashing               = false;
    float dashTimeRemaining      = 0.0f;
    float dashSpeed              = 20.0f;
    float3 dashTarget;
    float dashDistance = 3.0f;
    float dashDuration = 0.2f;
};