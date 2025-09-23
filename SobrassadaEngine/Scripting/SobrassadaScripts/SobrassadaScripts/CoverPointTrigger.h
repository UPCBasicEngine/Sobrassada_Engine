#pragma once
#include "Script.h"
#include "Standalone/Physics/CubeColliderComponent.h"

class GameObject;
class Archer;

class CoverPointTrigger : public Script
{
  public:
    CoverPointTrigger(GameObject* parent);
    virtual ~CoverPointTrigger() noexcept override { parent = nullptr; }

    bool Init() override;
    void Update(float deltaTime) override;
    void OnCollisionEnter(GameObject* otherObject, const float3 collisionNormal, ColliderLayer layer) override;
    void OnCollisionExit(GameObject* otherObject, ColliderLayer layer) override;

    // Core functionality
    void NotifyArchersCompromised();
    void RegisterWithArchers();
    bool IsCompromised() const { return isCompromised; }

    // List management
    void MoveCoverPointToOccupied();
    void MoveCoverPointToAvailable();
    void AddToGlobalAvailableList();

    // State management
    void CompromiseCoverPoint();
    void ResetCoverPoint();

    // Position utilities
    void CalculateGroundPosition();
    float3 GetGroundPosition() const;
    float3 GetFlankingPosition(const float3& playerPos) const;

  private:
    // Configuration
    std::string playerName = "CuChulainn";
    float compromiseRadius = 3.0f;
    float resetDelay       = 5.0f;

    // Runtime state
    GameObject* player     = nullptr;
    bool isCompromised     = false;
    float resetTimer       = 0.0f;
    float3 groundPosition  = float3::zero;

    // Archer management
    std::vector<Archer*> registeredArchers;
    bool isProjected    =  false;
};