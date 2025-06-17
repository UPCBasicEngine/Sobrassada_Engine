#pragma once

#include "Component.h"

#include "Math/float3.h"
#include <vector>

namespace math
{
    class float3;
}

class dtNavMeshQuery;

class SOBRASADA_API_ENGINE AIAgentComponent : public Component
{

  public:
    AIAgentComponent(UID uid, GameObject* parent);
    AIAgentComponent(const rapidjson::Value& initialState, GameObject* parent);
    ~AIAgentComponent() override;

    void Update(float deltaTime) override;
    void Render(float deltaTime) override;
    void RenderDebug(float deltaTime) override;
    void RenderEditorInspector() override;
    void Clone(const Component* other) override;
    void Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const override;
    void AddToCrowd();
    void RecreateAgent();
    void LookAtMovement(const float3& moveDir, float deltaTime);
    bool SetPathNavigation(const math::float3& destination, bool move = true);
    void PauseMovement();
    void ResumeMovement();

    float GetSpeed() const { return currentSpeed; }
    float GetAngularSpeed() const { return currentAngularSpeed; }
    unsigned int GetClosestPointInNavmesh(
        const float3& searchPos, const float3& searchArea, bool& posOverPoly, float3& closestPoint
    ) const;
    bool IsPaused() const { return isPaused; }

    void SetLookForward(bool look) { lookForward = look; }
    void SetSpeed(const float newSpeed, const float acceleration);
    void SetAngularSpeed(const float newAngular);
    void SetPosition(const float3& newPos);
    void ResetSpeed();
    void ResetAngularSpeed();

  private:
    float defaultSpeed           = 0.0f;
    float defaultAcceleration    = 0.0f;
    float currentSpeed           = 0.0f;
    float currentAcceleration    = 0.0f;
    float radius                 = 0.0f;
    float height                 = 0.0f;
    int agentId                  = -1; // Assigned by dtCrowd

    bool isPaused                = false;
    float3 frozenPosition        = float3::zero;
    float restoredSpeed          = 0.0f;
    float restoredAccel          = 0.0f;

    float maxAngularSpeed        = 0.0f;
    float currentAngularSpeed    = 0.0f;
    bool isRadians               = false;
    float restoreAngular         = 0.0f;

    dtNavMeshQuery* navMeshQuery = nullptr;

    bool lookForward             = false;
    float3 previousPos           = float3::zero;
};