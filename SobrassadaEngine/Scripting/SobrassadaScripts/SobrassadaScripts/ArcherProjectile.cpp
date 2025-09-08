#include "pch.h"
#include "ArcherProjectile.h"
#include "CameraComponent.h"
#include "Character.h"
#include "CuChulainn.h"
#include "GameObject.h"
#include "Math/Quat.h"
#include "ScriptComponent.h"
#include "Standalone/Physics/CapsuleColliderComponent.h"
#include "WallCollision.h"


static bool IsInsideCameraView(const float3& worldPosition, float screenEdgeMargin = 0.05f)
{
    Scene* scene                = AppEngine->GetSceneModule()->GetScene();
    CameraComponent* mainCamera = scene ? scene->GetMainCamera() : nullptr;
    if (!mainCamera) return true;

    const float4x4 view       = mainCamera->GetViewMatrix();
    const float4x4 projection = mainCamera->GetProjectionMatrix();

    const float4 clip         = projection * view * float4(worldPosition, 1.0f);
    if (fabsf(clip.w) < 0.0001f) return true;

    const float3 normalized = float3(clip.x, clip.y, clip.z) / clip.w;

    const float limit       = 1.0f + screenEdgeMargin;
    const bool insideX      = (normalized.x >= -limit) && (normalized.x <= limit);
    const bool insideY      = (normalized.y >= -limit) && (normalized.y <= limit);
    const bool insideZ      = (normalized.z >= -1.0f - screenEdgeMargin) && (normalized.z <= 1.0f + screenEdgeMargin);
    return insideX && insideY && insideZ;
}

ArcherProjectile::ArcherProjectile(GameObject* parent) : Script(parent)
{
    fields.push_back({"Speed", InspectorField::FieldType::Float, &speed, 0.0f, 100.0f});
    fields.push_back({"Range", InspectorField::FieldType::Float, &range, 0.0f, 100.0f});
    fields.push_back({"Damage", InspectorField::FieldType::Int, &damage, 0, 10});
}

bool ArcherProjectile::Init()
{
    collider = parent->GetComponent<CapsuleColliderComponent*>();
    if (!collider)
    {
        GLOG("[WARNING: ArcherProjectile Init()] Couldn't find the collider component");
        return false;
    }

    GLOG("ARCHER PROJECTILE INIT DEBUG");
    return true;
}

void ArcherProjectile::Update(float deltaTime)
{
    if (isStuckInWall)
    {
        stuckTimer += deltaTime;
        if (stuckTimer >= stuckDuration)
        {
            Reset();
        }
        return;
    }

    if (isActive)
    {
        Move(deltaTime);
    }
}

void ArcherProjectile::Shoot(const float3& origin, const float3& direction)
{
    // FIRST: Disable the arrow completely to prevent visual artifacts
    parent->SetEnabled(false);
    parent->SetEnabledRecursive(false);

    // SECOND: Set up the projectile state
    startPos                 = origin;
    this->direction          = direction.Normalized();
    frames                   = 0;
    isActive                 = true;

    // THIRD: Position and rotate the arrow while it's disabled
    const float3 scale       = parent->GetLocalTransform().ExtractScale();
    const Quat rotation      = Quat::LookAt(float3::unitZ, this->direction, float3::unitY, float3::unitY);
    const float4x4 transform = float4x4::FromTRS(origin, rotation, scale);
    parent->SetLocalTransform(transform);

    // FOURTH: Disable collider initially (will be enabled after frame delay)
    if (collider)
    {
        collider->SetEnabled(false);
    }

    // FIFTH: Enable the arrow after positioning is complete
    parent->SetEnabled(true);
    parent->SetEnabledRecursive(true);

    GLOG("Archer arrow shot initialized at position: (%.2f, %.2f, %.2f)", origin.x, origin.y, origin.z);
}

void ArcherProjectile::OnCollision(GameObject* otherObject, const float3 collisionNormal, ColliderLayer layer)
{
    // If collides with a character don't disable, do that in the character onCollision
    ScriptComponent* script = otherObject->GetComponent<ScriptComponent*>();
    if (script && script->GetScriptByType<Character>()) return;

    if (otherObject->HasTag(wallTag) && script->GetScriptByType<WallCollision>())
    {
        GLOG("WALL WALL WALL WALL");
        isStuckInWall = true;
        stuckTimer    = 0.0f;

        if (collider) collider->SetEnabled(false);

        GLOG("Arrow stuck in wall for %.2f seconds", stuckDuration);
        return;
    }
    parent->SetEnabled(false);
}

void ArcherProjectile::OnWallHit()
{
    GLOG("Archer projectile hit wall - activating stuck state");
    isStuckInWall = true;
    stuckTimer    = 0.0f;

    if (collider)
    {
        collider->SetEnabled(false);
    }

    GLOG("Archer arrow stuck in wall for %.2f seconds", stuckDuration);
}

void ArcherProjectile::Hit(GameObject* otherObject)
{
    ScriptComponent* script = otherObject->GetComponent<ScriptComponent*>();
    if (script && script->GetScriptByType<CuChulainn>())
    {
        CuChulainn* player = script->GetScriptByType<CuChulainn>();
        player->OnArrowHit();

        // TODO: Add archer-specific hit particles here
        GLOG("Archer arrow hit player - triggering particles");

        // Disable the arrow after hitting
        StopProjectile();
    }
}

void ArcherProjectile::Reset()
{
    isActive      = false;
    isStuckInWall = false;
    stuckTimer    = 0.0f;
    frames        = 0;
    hasHitTarget  = false;

    // Reset position and direction
    startPos      = float3::zero;
    direction     = float3::zero;

    // Disable collider
    if (collider)
    {
        collider->SetEnabled(false);
    }

    // Disable the GameObject
    parent->SetEnabled(false);

    GLOG("Archer projectile reset to initial state");
}

void ArcherProjectile::StopProjectile()
{
    Reset();
}

void ArcherProjectile::Move(float deltaTime)
{
    if (!isActive) return;

    // Let 20 frames pass before enabling the collider, so it doesn't collide with the shooter
    frames += 1;
    if (frames > 20 && collider && !collider->GetEnabled())
    {
        collider->SetEnabled(true);
    }

    float3 currentPos  = parent->GetPosition();
    currentPos        += direction * speed * deltaTime;
    parent->SetLocalPosition(currentPos);

    const float3 worldPos = parent->GetGlobalTransform().TranslatePart();

    // Check range first
    if (currentPos.Distance(startPos) > range)
    {
        GLOG("Archer arrow stopped - out of range");
        StopProjectile();
        return;
    }

    // Camera check with generous margin after arrow has been active for a bit
    if (frames > 60 && !IsInsideCameraView(worldPos, 0.5f))
    {
        GLOG("Archer arrow stopped - out of camera view");
        StopProjectile();
        return;
    }
}