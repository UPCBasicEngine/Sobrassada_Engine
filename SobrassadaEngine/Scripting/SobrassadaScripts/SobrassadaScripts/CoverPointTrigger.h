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

    
    void NotifyArchersCompromised();
    void RegisterWithArchers();
    bool IsCompromised() const { return isCompromised; }

    void MoveCoverPointToOccupied();
    void MoveCoverPointToAvailable();
    void AddToGlobalAvailableList();

    
    void CompromiseCoverPoint();
    void ResetCoverPoint();

  
    void CalculateGroundPosition();
    float3 GetGroundPosition() const;
    float3 GetFlankingPosition(const float3& playerPos) const;

  private:
    
    std::string playerName = "CuChulainn";
    float compromiseRadius = 3.0f;
   

    
    GameObject* player     = nullptr;
    bool isCompromised     = false;
   
    float3 groundPosition  = float3::zero;

    // Archer management
    std::vector<Archer*> registeredArchers;
    bool isProjected    =  false;
};