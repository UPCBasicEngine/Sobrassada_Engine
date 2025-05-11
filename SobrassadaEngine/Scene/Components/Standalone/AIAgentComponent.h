#pragma once

#include "Component.h"

#include "Math/float3.h"
#include <vector>

namespace math
{
    class float3;
}

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

    float GetSpeed() const { return speed; }
    bool IsPaused() const { return isPaused; }

  private:
    float speed           = 0.f;
    float radius          = 0.f;
    float height          = 0.f;
    int agentId           = -1; // Assigned by dtCrowd

    bool isPaused         = false;
    float3 frozenPosition   = float3::zero;
    float restoredSpeed   = 0.f;
    float restoredAccel     = 0.f;

    float maxAngularSpeed = 0.0f;
    bool isRadians        = false;
    float restoreAngular    = 0.f;

};