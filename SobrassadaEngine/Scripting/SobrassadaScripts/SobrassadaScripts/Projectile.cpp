#include "pch.h"

#include "Projectile.h"

#include "AttackVfxSpritesheet.h"
#include "CameraComponent.h"
#include "Character.h"
#include "CuChulainn.h"
#include "GameObject.h"
#include "ScriptComponent.h"
#include "ShaderScriptComponent.h"
#include "Standalone/MeshComponent.h"
#include "Standalone/Physics/CapsuleColliderComponent.h"
#include "Standalone/Audio/AudioSourceComponent.h"
#include "Wwise_IDs.h"

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

    fields.push_back({"Spritesheet vertical name", InspectorField::FieldType::InputText, &spritesheetNameV});
    fields.push_back({"Spritesheet horizotnal name", InspectorField::FieldType::InputText, &spritesheetNameH});
}

bool Projectile::Init()
{
    collider = parent->GetComponent<CapsuleColliderComponent*>();
    if (!collider)
    {
        GLOG("[WARNING: Projectile Init()] Couldn't find the collider component");
        return false;
    }
    audio                         = parent->GetComponent<AudioSourceComponent*>();
    if (!audio) GLOG("[WARNING: Projectile Init()] Couldn't find the audio source component");

    GameObject* spritesheetObject = parent->GetChildGameObjectByName(spritesheetNameV);
    if (spritesheetObject)
    {
        spritesheetMeshV = spritesheetObject->GetComponent<MeshComponent*>();
        if (!spritesheetMeshV) GLOG("[WARNING: Projectile Init()] Couldn't find the mesh component");

        ShaderScriptComponent* shader = spritesheetObject->GetComponent<ShaderScriptComponent*>();
        if (shader)
        {
            spritesheetV = shader->GetScriptByType<AttackVfxSpritesheet>();
        }
    }
    if (!spritesheetV) GLOG("[WARNING: Projectile Init()] Couldn't find the spritesheet component");

    spritesheetObject = parent->GetChildGameObjectByName(spritesheetNameH);
    if (spritesheetObject)
    {
        spritesheetMeshH = spritesheetObject->GetComponent<MeshComponent*>();
        if (!spritesheetMeshH) GLOG("[WARNING: Projectile Init()] Couldn't find the mesh component");

        ShaderScriptComponent* shader = spritesheetObject->GetComponent<ShaderScriptComponent*>();
        if (shader)
        {
            spritesheetH = shader->GetScriptByType<AttackVfxSpritesheet>();
        }
    }
    if (!spritesheetH) GLOG("[WARNING: Projectile Init()] Couldn't find the spritesheet component");

    return true;
}

void Projectile::Update(float deltaTime)
{
    Move(deltaTime);
}

void Projectile::Shoot(const float3& origin, const float3& direction)
{
    startPos        = origin + (float3::unitY * 0.5f);
    this->direction = direction.Normalized();
    frames          = 0;
    if (collider) collider->SetEnabled(false);
    if (audio) audio->EmitEvent(AK::EVENTS::PLAY_SFX_MC_RANGEATTACK);

    // Rotate spear object
    const float3 scale       = parent->GetLocalTransform().ExtractScale();
    const Quat rotation      = Quat::LookAt(float3::unitZ, this->direction, float3::unitY, float3::unitY);
    const float4x4 transform = float4x4::FromTRS(startPos, rotation, scale);

    const float4x4 parentWS  = parent->GetParentGlobalTransform();
    const float4x4 localTRS  = parentWS.Inverted() * transform;

    parent->SetLocalTransform(localTRS);

    parent->SetEnabledRecursive(true);

    if (spritesheetMeshV) spritesheetMeshV->SetEnabled(false);
    if (spritesheetMeshH) spritesheetMeshH->SetEnabled(false);
    if (spritesheetV) spritesheetV->Reset();
    if (spritesheetH) spritesheetH->Reset();
}

void Projectile::OnCollision(GameObject* otherObject, const float3 collisionNormal, ColliderLayer layer)
{
    // GLOG("Collision in projectile with: %s", otherObject->GetName().c_str());

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

    // float3 currentPos  = parent->GetPosition();
    // currentPos        += direction * speed * deltaTime;

    const float3 worldPos   = parent->GetGlobalTransform().TranslatePart() + direction * speed * deltaTime;
    const float4x4 parentWS = parent->GetParentGlobalTransform();
    const float3 nextLocal  = (parentWS.Inverted() * float4(worldPos, 1.0f)).xyz();

    parent->SetLocalPosition(nextLocal);

    if (worldPos.Distance(startPos) > range || !IsInsideCameraView(worldPos, 0.0f))
    {
        parent->SetEnabled(false);
    }
}
