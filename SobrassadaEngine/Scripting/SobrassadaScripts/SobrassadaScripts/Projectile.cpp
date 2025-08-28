#include "pch.h"

#include "Projectile.h"

#include "Character.h"
#include "GameObject.h"
#include "ScriptComponent.h"
#include "CuChulainn.h"
#include "CameraComponent.h"
#include "Standalone/Physics/CapsuleColliderComponent.h"

#include "Math/Quat.h"

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

Projectile::Projectile(GameObject* parent) : Script(parent)
{
    fields.push_back({"Speed", InspectorField::FieldType::Float, &speed, 0.0f, 100.0f});
    fields.push_back({"Range", InspectorField::FieldType::Float, &range, 0.0f, 100.0f});
    fields.push_back({"Damage", InspectorField::FieldType::Int, &damage, 0, 10});
}

bool Projectile::Init()
{
    collider = parent->GetComponent<CapsuleColliderComponent*>();
    if (!collider)
    {
        GLOG("[WARNING: Projectile Init()] Couldn't find the collider component");
        return false;
    }
    return true;
}

void Projectile::Update(float deltaTime)
{
    Move(deltaTime);
}

void Projectile::Shoot(const float3& origin, const float3& direction)
{
   
    startPos        = origin;
    this->direction = direction;
    frames          = 0;
    
    // Rotate spear object
    const float3 scale       = parent->GetLocalTransform().ExtractScale();
    const Quat rotation      = Quat::LookAt(float3::unitZ, direction, float3::unitY, float3::unitY);
    const float4x4 transform = float4x4::FromTRS(origin, rotation, scale);
    parent->SetLocalTransform(transform);

    parent->SetEnabledRecursive(true);
}

void Projectile::OnCollision(GameObject* otherObject, const float3 collisionNormal, ColliderLayer layer)
{
    //GLOG("Collision in projectile with: %s", otherObject->GetName().c_str());

    // If collides with a character don't disable, do that in the character onCollision
    ScriptComponent* script = otherObject->GetComponent<ScriptComponent*>();
  if (script && script->GetScriptByType<Character>()) return;

      parent->SetEnabled(false);
}

void Projectile::Hit(GameObject* otherObject)
{
    ScriptComponent* script = otherObject->GetComponent<ScriptComponent*>();
    if (script && script->GetScriptByType<CuChulainn>())
    {
        CuChulainn* player = script->GetScriptByType<CuChulainn>();
        player->OnArrowHit();
    }
}

void Projectile::Move(float deltaTime)
{
    // Let 20 frames pass before enabling the collider, so it doesn't collide with the previous collided element.
    // TODO: Try to change this
    frames += 1;
    if (frames > 20 && collider && !collider->GetEnabled()) collider->SetEnabled(true);

    float3 currentPos  = parent->GetPosition();
    currentPos        += direction * speed * deltaTime;
    parent->SetLocalPosition(currentPos);

    const float3 worldPos = parent->GetGlobalTransform().TranslatePart();

    if (currentPos.Distance(startPos) > range || !IsInsideCameraView(worldPos, 0.11f))
    {
        parent->SetEnabled(false);
    }
}
