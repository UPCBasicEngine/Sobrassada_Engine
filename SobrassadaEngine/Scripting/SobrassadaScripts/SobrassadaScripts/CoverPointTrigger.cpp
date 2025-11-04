#include "pch.h"

#include "CoverPointTrigger.h"
#include "Application.h"
#include "Archer.h"
#include "CuChulainn.h"
#include "GameObject.h"
#include "Geometry/LineSegment.h"
#include "RaycastController.h"
#include "Scene.h"
#include "SceneModule.h"
#include "ScriptComponent.h"
#include "Standalone/Physics/CubeColliderComponent.h"
#include "Standalone/AIAgentComponent.h"


CoverPointTrigger::CoverPointTrigger(GameObject* parent) : Script(parent)
{
    fields.push_back({"Player Name", InspectorField::FieldType::InputText, &playerName});
    fields.push_back({"Compromise Radius", InspectorField::FieldType::Float, &compromiseRadius, 1.0f, 10.0f});
    fields.push_back({"Reset Delay", InspectorField::FieldType::Float, &resetDelay, 1.0f, 30.0f});
}

bool CoverPointTrigger::Init()
{
    player = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(playerName);
    if (!player)
    {
        GLOG("[WARNING] CoverPointTrigger: No player found by name '%s'", playerName.c_str());
        return false;
    }

    
    CubeColliderComponent* collider = parent->GetComponent<CubeColliderComponent*>();
    if (!collider)
    {
        GLOG("[WARNING] CoverPointTrigger: No CubeCollider found on %s", parent->GetName().c_str());
        return false;
    }

  
    collider->generateCallback = true;
    collider->colliderType     = ColliderType::TRIGGER;

    
    float3 triggerSize         = float3(compromiseRadius, compromiseRadius * 0.5f, compromiseRadius);
    collider->size             = triggerSize;

   
    CalculateGroundPosition();

  
    RegisterWithArchers();

  
    AddToGlobalAvailableList();

    /*GLOG(
        "CoverPointTrigger initialized for %s with radius %.2f at ground pos (%.2f, %.2f, %.2f)",
        parent->GetName().c_str(), compromiseRadius, groundPosition.x, groundPosition.y, groundPosition.z
    );*/

    return true;
}

void CoverPointTrigger::CalculateGroundPosition()
{
    float3 coverPointPos = parent->GetPosition();
    GLOG("=== CALCULATING GROUND POSITION FOR %s ===", parent->GetName().c_str());

    groundPosition = coverPointPos;
    isProjected    = false;

    GLOG("Ground position set to original: (%.2f, %.2f, %.2f)", groundPosition.x, groundPosition.y, groundPosition.z);
}

void CoverPointTrigger::RegisterWithArchers()
{
    const std::vector<GameObject*>* archerObjects =
        AppEngine->GetSceneModule()->GetScene()->GetTaggedGameObjects(HashString("Archer"));

    if (!archerObjects)
    {
        GLOG("[WARNING] No GameObjects with 'Archer' tag found");
        return;
    }

    for (GameObject* obj : *archerObjects)
    {
        if (!obj || !obj->IsEnabled()) continue;

        ScriptComponent* scriptComp = obj->GetComponent<ScriptComponent*>();
        if (scriptComp)
        {
            Archer* archer = scriptComp->GetScriptByType<Archer>();
            if (archer)
            {
                registeredArchers.push_back(archer);
                //GLOG("Registered cover point %s with archer %s", parent->GetName().c_str(), obj->GetName().c_str());
            }
        }
    }

    //GLOG("Cover point %s registered with %d archers", parent->GetName().c_str(), registeredArchers.size());
}

void CoverPointTrigger::AddToGlobalAvailableList()
{
    
    for (Archer* archer : registeredArchers)
    {
        if (archer)
        {
            //std::vector<GameObject*>& availablePoints = archer->GetAvailableCoverPoints();

            
            bool alreadyExists                        = false;
           /* for (GameObject* existingPoint : availablePoints)
            {
                if (existingPoint == parent)
                {
                    alreadyExists = true;
                    break;
                }
            }

            if (!alreadyExists)
            {
                availablePoints.push_back(parent);
                GLOG("Added cover point %s to archer's available list", parent->GetName().c_str());
            }*/
        }
    }
}

void CoverPointTrigger::MoveCoverPointToOccupied()
{
    //GLOG("Moving cover point %s to OCCUPIED list", parent->GetName().c_str());

    for (Archer* archer : registeredArchers)
    {
        if (!archer) continue;

        // std::vector<GameObject*>& availablePoints = archer->GetAvailableCoverPoints();
        // std::vector<GameObject*>& occupiedPoints  = archer->GetOccupiedCoverPoints();
    }
}

void CoverPointTrigger::MoveCoverPointToAvailable()
{
    //GLOG("Moving cover point %s back to AVAILABLE list", parent->GetName().c_str());

    for (Archer* archer : registeredArchers)
    {
        if (!archer) continue;

        //std::vector<GameObject*>& availablePoints = archer->GetAvailableCoverPoints();
        //std::vector<GameObject*>& occupiedPoints  = archer->GetOccupiedCoverPoints();

       
       

     
        //auto availableIt = std::find(availablePoints.begin(), availablePoints.end(), parent);
        
    }
}

void CoverPointTrigger::Update(float deltaTime)
{
    if (!player) return;

  
    if (!isCompromised)
    {
        float distanceToPlayer = groundPosition.Distance(player->GetPosition());
        if (distanceToPlayer <= compromiseRadius)
        {
            /*GLOG(
                "Player manually detected at cover point %s (distance: %.2f)", parent->GetName().c_str(),
                distanceToPlayer
            );*/
            CompromiseCoverPoint();
        }
    }

   if (isCompromised)
    {
        float distanceToPlayer = groundPosition.Distance(player->GetPosition());
        if (distanceToPlayer > compromiseRadius * 1.5f)
        {
            //GLOG("Player far from cover point %s - RESETTING", parent->GetName().c_str());
            ResetCoverPoint();
        }
    }
}

void CoverPointTrigger::CompromiseCoverPoint()
{
    if (isCompromised) return; 
    //GLOG("COMPROMISING cover point %s", parent->GetName().c_str());

    isCompromised = true;
   
    MoveCoverPointToOccupied();

  
    NotifyArchersCompromised();
}

void CoverPointTrigger::ResetCoverPoint()
{
    if (!isCompromised) return;

    //GLOG("RESETTING cover point %s", parent->GetName().c_str());

    isCompromised = false;

   
    MoveCoverPointToAvailable();
}

void CoverPointTrigger::OnCollisionEnter(GameObject* otherObject, const float3 collisionNormal, ColliderLayer layer)
{
    if (otherObject != player)
    {
        return;
    }

    ScriptComponent* scriptComp = player->GetComponent<ScriptComponent*>();
    if (scriptComp)
    {
        CuChulainn* playerScript = scriptComp->GetScriptByType<CuChulainn>();
        if (playerScript)
        {
            //GLOG("Player collision entered cover point %s", parent->GetName().c_str());
            CompromiseCoverPoint();
        }
    }
}

void CoverPointTrigger::OnCollisionExit(GameObject* otherObject, ColliderLayer layer)
{
    if (otherObject != player) return;

    ScriptComponent* scriptComp = player->GetComponent<ScriptComponent*>();
    if (scriptComp)
    {
        CuChulainn* playerScript = scriptComp->GetScriptByType<CuChulainn>();
        if (playerScript)
        {
            //GLOG("Player collision exited cover point %s", parent->GetName().c_str());
            ResetCoverPoint();
        }
    }
}

void CoverPointTrigger::NotifyArchersCompromised()
{
    for (Archer* archer : registeredArchers)
    {
        if (!archer) continue;

    }
}


float3 CoverPointTrigger::GetGroundPosition() const
{
    if (!isProjected)
    {
        const_cast<CoverPointTrigger*>(this)->CalculateGroundPosition();
    }

    return groundPosition;
}

float3 CoverPointTrigger::GetFlankingPosition(const float3& playerPos) const
{
    
    float3 coverToPlayer = (playerPos - groundPosition).Normalized();
    coverToPlayer.y      = 0; 

    
    float3 perpendicular = float3(-coverToPlayer.z, 0, coverToPlayer.x);

  
    float3 leftFlank     = groundPosition + perpendicular * 4.0f;
    float3 rightFlank    = groundPosition - perpendicular * 4.0f;

    float leftDistance   = leftFlank.Distance(playerPos);
    float rightDistance  = rightFlank.Distance(playerPos);

    return (leftDistance > rightDistance) ? leftFlank : rightFlank;
}