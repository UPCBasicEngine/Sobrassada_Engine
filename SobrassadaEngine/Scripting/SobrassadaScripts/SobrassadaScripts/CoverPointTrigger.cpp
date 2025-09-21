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

    // Ensure GameObject has CubeCollider
    CubeColliderComponent* collider = parent->GetComponent<CubeColliderComponent*>();
    if (!collider)
    {
        GLOG("[WARNING] CoverPointTrigger: No CubeCollider found on %s", parent->GetName().c_str());
        return false;
    }

    // Configure collider as trigger
    collider->generateCallback = true;
    collider->colliderType     = ColliderType::TRIGGER;

    // Set trigger size based on compromiseRadius
    float3 triggerSize         = float3(compromiseRadius, compromiseRadius * 0.5f, compromiseRadius);
    collider->size             = triggerSize;

    // Calculate ground position using raycast
    CalculateGroundPosition();

    // Register with all archers in scene
    RegisterWithArchers();

    // Add this cover point to global available list
    AddToGlobalAvailableList();

    GLOG(
        "CoverPointTrigger initialized for %s with radius %.2f at ground pos (%.2f, %.2f, %.2f)",
        parent->GetName().c_str(), compromiseRadius, groundPosition.x, groundPosition.y, groundPosition.z
    );

    return true;
}

void CoverPointTrigger::CalculateGroundPosition()
{
    float3 coverPointPos = parent->GetPosition();
    GLOG("=== CALCULATING GROUND POSITION FOR %s ===", parent->GetName().c_str());

    // NO proyectes aquí - hazlo cuando sea necesario
    groundPosition = coverPointPos;
    isProjected    = false; // Marca que no está proyectado aún

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
                GLOG("Registered cover point %s with archer %s", parent->GetName().c_str(), obj->GetName().c_str());
            }
        }
    }

    GLOG("Cover point %s registered with %d archers", parent->GetName().c_str(), registeredArchers.size());
}

void CoverPointTrigger::AddToGlobalAvailableList()
{
    // Add this cover point to all archers' available lists
    for (Archer* archer : registeredArchers)
    {
        if (archer)
        {
            std::vector<GameObject*>& availablePoints = archer->GetAvailableCoverPoints();

            // Check if already in list to avoid duplicates
            bool alreadyExists                        = false;
            for (GameObject* existingPoint : availablePoints)
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
            }
        }
    }
}

void CoverPointTrigger::MoveCoverPointToOccupied()
{
    GLOG("Moving cover point %s to OCCUPIED list", parent->GetName().c_str());

    for (Archer* archer : registeredArchers)
    {
        if (!archer) continue;

        std::vector<GameObject*>& availablePoints = archer->GetAvailableCoverPoints();
        std::vector<GameObject*>& occupiedPoints  = archer->GetOccupiedCoverPoints();

        // Remove from available list
        auto it                                   = std::find(availablePoints.begin(), availablePoints.end(), parent);
        if (it != availablePoints.end())
        {
            availablePoints.erase(it);
            GLOG("Removed from available list for archer");
        }

        // Add to occupied list if not already there
        auto occupiedIt = std::find(occupiedPoints.begin(), occupiedPoints.end(), parent);
        if (occupiedIt == occupiedPoints.end())
        {
            occupiedPoints.push_back(parent);
            GLOG("Added to occupied list for archer");
        }
    }
}

void CoverPointTrigger::MoveCoverPointToAvailable()
{
    GLOG("Moving cover point %s back to AVAILABLE list", parent->GetName().c_str());

    for (Archer* archer : registeredArchers)
    {
        if (!archer) continue;

        std::vector<GameObject*>& availablePoints = archer->GetAvailableCoverPoints();
        std::vector<GameObject*>& occupiedPoints  = archer->GetOccupiedCoverPoints();

        // Remove from occupied list
        auto it                                   = std::find(occupiedPoints.begin(), occupiedPoints.end(), parent);
        if (it != occupiedPoints.end())
        {
            occupiedPoints.erase(it);
            GLOG("Removed from occupied list for archer");
        }

        // Add back to available list if not already there
        auto availableIt = std::find(availablePoints.begin(), availablePoints.end(), parent);
        if (availableIt == availablePoints.end())
        {
            availablePoints.push_back(parent);
            GLOG("Added back to available list for archer");
        }
    }
}

void CoverPointTrigger::Update(float deltaTime)
{
    if (!player) return;

    // Manual distance check as backup to collision detection
    if (!isCompromised)
    {
        float distanceToPlayer = groundPosition.Distance(player->GetPosition());
        if (distanceToPlayer <= compromiseRadius)
        {
            GLOG(
                "Player manually detected at cover point %s (distance: %.2f)", parent->GetName().c_str(),
                distanceToPlayer
            );
            CompromiseCoverPoint();
        }
    }

   if (isCompromised)
    {
        float distanceToPlayer = groundPosition.Distance(player->GetPosition());
        if (distanceToPlayer > compromiseRadius * 1.5f)
        {
            GLOG("Player far from cover point %s - RESETTING", parent->GetName().c_str());
            ResetCoverPoint();
        }
    }
}

void CoverPointTrigger::CompromiseCoverPoint()
{
    if (isCompromised) return; // Already compromised

    GLOG("COMPROMISING cover point %s", parent->GetName().c_str());

    isCompromised = true;
    // Move to occupied list
    MoveCoverPointToOccupied();

    // Notify all registered archers
    NotifyArchersCompromised();
}

void CoverPointTrigger::ResetCoverPoint()
{
    if (!isCompromised) return; // Not compromised

    GLOG("RESETTING cover point %s", parent->GetName().c_str());

    isCompromised = false;

    // Move back to available list
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
            GLOG("Player collision entered cover point %s", parent->GetName().c_str());
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
            GLOG("Player collision exited cover point %s", parent->GetName().c_str());
            ResetCoverPoint();
        }
    }
}

void CoverPointTrigger::NotifyArchersCompromised()
{
    for (Archer* archer : registeredArchers)
    {
        if (!archer) continue;

        // If this archer is using this cover point, force search for new one
        if (archer->GetCurrentCoverPoint() == parent)
        {
            GLOG("Notifying archer that cover point %s is compromised", parent->GetName().c_str());
            archer->ForceNewCoverPoint();
        }
    }
}


float3 CoverPointTrigger::GetGroundPosition() const
{
    if (!isProjected && !registeredArchers.empty())
    {
        Archer* archer = registeredArchers[0];
        if (archer && archer->GetAI())
        {
            bool posOverPoly        = false;
            float3 navPosition      = float3::zero;
            const float3 searchArea = {5.0f, 10.0f, 5.0f};

            archer->GetAI()->GetClosestPointInNavmesh(groundPosition, searchArea, posOverPoly, navPosition);

            if (posOverPoly)
            {
                const_cast<CoverPointTrigger*>(this)->groundPosition = navPosition;
                const_cast<CoverPointTrigger*>(this)->isProjected    = true;
                GLOG(
                    "LAZY PROJECTED %s to: (%.2f, %.2f, %.2f)", parent->GetName().c_str(), navPosition.x, navPosition.y,
                    navPosition.z
                );
            }
        }
    }

    return groundPosition;
}

float3 CoverPointTrigger::GetFlankingPosition(const float3& playerPos) const
{
    // Calculate perpendicular positions for flanking
    float3 coverToPlayer = (playerPos - groundPosition).Normalized();
    coverToPlayer.y      = 0; // Keep on ground plane

    // Cross product with up vector to get perpendicular
    float3 perpendicular = float3(-coverToPlayer.z, 0, coverToPlayer.x);

    // Try both sides and pick the one farther from player
    float3 leftFlank     = groundPosition + perpendicular * 4.0f;
    float3 rightFlank    = groundPosition - perpendicular * 4.0f;

    float leftDistance   = leftFlank.Distance(playerPos);
    float rightDistance  = rightFlank.Distance(playerPos);

    return (leftDistance > rightDistance) ? leftFlank : rightFlank;
}