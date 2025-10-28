#pragma once

#include "Component.h"

#include "Math/float3.h"

class dtNavMeshQuery;

using dtPolyRef                           = unsigned int;

constexpr const char* DashBlockerGOTags[] = {"MagicBarrier"};

class SOBRASADA_API_ENGINE CharacterControllerComponent : public Component
{

  public:
    CharacterControllerComponent(UID uid, GameObject* parent);
    CharacterControllerComponent(const rapidjson::Value& initialState, GameObject* parent);

    ~CharacterControllerComponent() override;
    void Init() override;
    void Update(float deltaTime) override;
    void RenderDebug(float deltaTime) override;
    void RenderEditorInspector() override;
    void Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const override;
    void Clone(const Component* other) override;

    void AdjustHeightToNavMesh(float3 currentPos, float deltaTime);
    void Move(float deltaTime);
    void LookAtMovement(const float3& moveDir, float deltaTime);
    void Rotate(float rotationDirection, float deltaTime);
    void SetDirection(const float3& direction);
    void LookAt(const float3& direction);
    void MoveTo(float speed);

    const float3& GetTargetDirection() const { return targetDirection; }
    const float3& GetFrontDirection() const { return rotateDirection; }
    const float3& GetLastPosition() const { return lastPosition; }
    const float& GetSpeed() const { return currentSpeed; }
    const float& GetMaxSpeed() const { return maxSpeed; }
    bool IsDashing() const { return isDashing; }
    float2 GetRealSpeed() const;
    bool IsGrounded() { return isGrounded; }
    bool GetInputDown() const { return inputDown; }
    float GetDashDuration() const { return dashDuration; }

    void SetTargetDirection(float3 newTargetDirection) { targetDirection = newTargetDirection; }
    void SetMaxSpeed(float newSpeed) { maxSpeed = newSpeed; }
    void SetInputDown(bool input) { inputDown = input; }
    void EnableMovement(bool enable) { movementEnabled = enable; }
    void StartDash();
    void EndDash() { isDashing = false; }

    void SetIsRunning(bool running) { isRunning = running; }

    const float3& GetVelocity() const { return currentVelocity; }
    bool IsMoving(float threshold = 0.1f) const { return currentVelocity.Length() > threshold; }
    float3 GetPredictedPosition(float timeAhead) const { return lastPosition + (currentVelocity * timeAhead); }
    float3 GetDashDirection() const { return dashDirection; }

  private:
    void Dash(float deltaTime);
    void CheckDashObstacles();
    unsigned int GetClosestPointInNavmesh(
        const float3& searchPos, const float3& searchArea, bool& posOverPoly, float3& closestPoint
    ) const;

  private:
    float3 targetDirection                   = float3::zero;
    float3 lastPosition                      = float3::zero;
    float3 previousPosition                  = float3::zero;
    float3 currentVelocity                   = float3::zero;
    static const int VELOCITY_SAMPLES        = 3;
    float3 velocitySamples[VELOCITY_SAMPLES] = {float3::zero};
    int velocitySampleIndex                  = 0;

    float walkSpeed                          = 1.0f;
    float maxSpeed                           = 7.0f;
    float maxAngularSpeed                    = 0.0f;
    float acceleration                       = 10.0f;
    float currentSpeed                       = 0.0f;

    bool isRadians                           = false;

    dtNavMeshQuery* navMeshQuery             = nullptr;

    float gravity                            = -40.0f;
    float verticalSpeed                      = 0.0f;
    float maxFallSpeed                       = -30.0f;

    bool inputDown                           = true;
    bool isRotating                          = false;
    float3 targetLookDirection               = float3::zero;

    float3 rotateDirection                   = float3::unitZ;
    bool movementEnabled                     = true;
    bool isGrounded                          = false;

    bool isDashing                           = false;
    float dashTimeRemaining                  = 0.0f;
    float dashSpeed                          = 20.0f;
    float3 dashDirection                     = float3::zero;
    float dashDistance                       = 6.0f;
    float dashDuration                       = 0.3f;
    bool dashToNavmesh                       = false;
    bool obstacleInDash                      = false;

    bool isRunning                           = false;
    bool preciseDash                         = true;
    bool dashMoveBlocked                     = false;
};