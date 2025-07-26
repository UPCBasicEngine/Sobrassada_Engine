#include "pch.h"
#undef max
#undef min

#include "Application.h"
#include "CameraComponent.h"
#include "CameraMovement.h"
#include "CuChulainn.h"
#include "FireballTrap.h"
#include "GameObject.h"
#include "ScriptComponent.h"
#include "Standalone/CharacterControllerComponent.h"
#include "Standalone/MeshComponent.h"
#include "Standalone/Physics/CubeColliderComponent.h"
#include "Standalone/Physics/SphereColliderComponent.h"

#include <algorithm>
#include <random>

FireballTrap::FireballTrap(GameObject* parent) : Script(parent)
{
    SetupInspectorFields(); // everything Inspector‑exposed is registered here
}

void FireballTrap::SetupInspectorFields()
{
    // General settings
    fields.push_back({"Activation Range", InspectorField::FieldType::Float, &cfg.activationRange, 0.0f, 100.0f});
    fields.push_back({"Min Attack Cooldown", InspectorField::FieldType::Float, &cfg.minAttackCooldown, 0.0f, 10.0f});
    fields.push_back({"Max Attack Cooldown", InspectorField::FieldType::Float, &cfg.maxAttackCooldown, 0.0f, 30.0f});

    // Damage
    fields.push_back({"Trap Damage", InspectorField::FieldType::Int, &cfg.impactDamage, 0, 5});
    fields.push_back({"Damage Duration", InspectorField::FieldType::Float, &cfg.bigBurnDuration, 0.0f, 10.0f});

    // Physics
    fields.push_back({"Rotation Speed", InspectorField::FieldType::Float, &cfg.rotationSpeed, 0.0f, 100.0f});
    fields.push_back({"Falling Height", InspectorField::FieldType::Float, &cfg.fallingHeight, 0.0f, 200.0f});
    fields.push_back({"Max Fall Speed", InspectorField::FieldType::Float, &cfg.maxFallSpeed, 0.0f, 100.0f});
    fields.push_back({"Gravity", InspectorField::FieldType::Float, &cfg.gravity, 0.0f, 20.0f});

    // Mini fireballs
    fields.push_back({"Mini Prototype", InspectorField::FieldType::GameObject, &miniPrototype, 0.f, 0.f});
    fields.push_back({"Mini Count", InspectorField::FieldType::Int, &miniCount, 1.f, 12.f});
    fields.push_back({"Mini Speed", InspectorField::FieldType::Float, &miniSpeed, 1.f, 30.f});
    fields.push_back({"Mini Lifetime", InspectorField::FieldType::Float, &miniLifeTime, 0.f, 10.f});

    // Impact decals
    fields.push_back({"Impact Prefab", InspectorField::FieldType::GameObject, &impactPrefab, 0.f, 0.f});

    // Arc
    fields.push_back({"Max Launch Radius", InspectorField::FieldType::Float, &cfg.maxLaunchRadius, 0.f, 20.f});
    fields.push_back({"Direction arc", InspectorField::FieldType::Float, &cfg.launchYawDeg, -180.f, 180.f});

    // Fall indicator
    fields.push_back({"Landing Indicator", InspectorField::FieldType::GameObject, &indicatorPrefab, 0.f, 0.f});
    fields.push_back({"Indicator Scale", InspectorField::FieldType::Float, &indicatorScale, 0.1f, 2.0f});
}

bool FireballTrap::Init()
{
    spawnZone = parent->GetComponent<CubeColliderComponent*>();
    if (!spawnZone)
    {
        GLOG("[WARNING] FireballTrap: Spawn zone CubeCollider not found");
    }
    else
    {
        spawnHalfSize               = spawnZone->size * 0.5f;
        spawnCenter                 = spawnZone->centerOffset;

        spawnZone->generateCallback = false;
        spawnZone->colliderType     = ColliderType::TRIGGER;
        spawnZone->layer            = ColliderLayer::WORLD_OBJECTS;
        spawnZone->SetEnabled(false);
    }

    groundMesh = parent->GetComponent<MeshComponent*>();
    if (groundMesh) groundMesh->SetEnabled(false);
    else GLOG("[WARNING] FireballTrap without mesh component.");

    damageCollider = parent->GetComponent<SphereColliderComponent*>();
    if (damageCollider) damageCollider->SetEnabled(false);
    else GLOG("[WARNING] FireballTrap without sphere collider component.");

    if (!parent->GetChildren().empty())
    {
        fireball = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(parent->GetChildren()[0]);
        if (fireball) fireball->SetEnabled(false);
        else GLOG("[WARNING] No fireball found as child of base");
    }
    if (parent->GetChildren().size() > 1)
    {
        fireballShadow = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(parent->GetChildren()[1]);
        if (fireballShadow) fireballShadow->SetEnabled(false);
        else GLOG("[WARNING] No fireball shadow found as child of base");
        shadowBaseScale = fireballShadow->GetScale();
    }

    baseLocal = parent->GetLocalTransform();

    if (miniPrototype) miniPrototype->SetEnabled(false);
    else GLOG("[WARNING] FireballTrap: Mini prototype reference not set");

    if (impactPrefab) impactPrefab->SetEnabled(false);
    else GLOG("[WARNING] FireballTrap: Impact prefab reference not set");

    shakeCam = FindShakeCamera();
    if (!shakeCam) GLOG("[WARNING] FireballTrap: CameraMovement not found");

    if (indicatorPrefab) indicatorPrefab->SetEnabled(false);
    else GLOG("[WARNING] FireballTrap: landing indicator prefab not set");

    return true;
}

void FireballTrap::Update(float deltaTime)
{
    if (!character || !groundMesh || !damageCollider || !fireball) return; // missing refs

    switch (activationState)
    {
    case ACTIVATION_STATE::SLEEPING:
    {
        float distanceSq = character->GetLastPosition().DistanceSq(parent->GetGlobalTransform().TranslatePart());
        if (distanceSq <= cfg.activationRange * cfg.activationRange)
        {
            randomAttackTime = GenerateRandomAttackTime(cfg.minAttackCooldown, cfg.maxAttackCooldown);
            activatedTime    = 0.f;
            activationState  = ACTIVATION_STATE::IDLE;
        }
        break;
    }
    case ACTIVATION_STATE::IDLE:
        activatedTime += deltaTime;
        if (activatedTime >= randomAttackTime) StartAttack();
        break;
    case ACTIVATION_STATE::DROPPING:
        UpdateFireball(deltaTime);
        break;
    case ACTIVATION_STATE::DAMAGING:
        impactElapsed += deltaTime;
        if (impactElapsed >= cfg.bigBurnDuration) DisableDamage();
        break;
    }

    UpdateMinis(deltaTime);

    if (activeIndicator)
    {
        indicatorPulse += deltaTime * 4.0f;
        float s         = 1.0f + 0.25f * sinf(indicatorPulse);

        float3 pos      = activeIndicator->GetLocalTransform().TranslatePart();
        float3 scale    = indicatorBaseScale * s;

        activeIndicator->SetLocalTransform(float4x4::FromTRS(pos, float3x3::identity, scale));
    }

    for (auto it = activeMiniDecals.begin(); it != activeMiniDecals.end();)
    {
        it->timer -= deltaTime;
        if (it->timer <= 0.f)
        {
            RecycleGO(it->go);
            it = activeMiniDecals.erase(it);
        }
        else ++it;
    }
}
void FireballTrap::StartAttack()
{
    parent->SetLocalTransform(baseLocal); // root never moves -> reset

    // Random IMPACT inside collider
    float3 impactWorld    = RandomSpawnPoint();
    float3 impactLocal    = parent->GetGlobalTransform().Inverted().MulPos(impactWorld);
    impactLocal.y         = 0.f;
    impactOffsetLocal     = impactLocal;
    lastImpactWorld       = impactWorld; 

    // Create indicator
    float3 indicatorLocal = parent->GetGlobalTransform().Inverted().MulPos(impactWorld);
    activeIndicator       = SpawnIndicator(indicatorLocal, cfg.bigBurnRadius * indicatorScale);
    if (activeIndicator) indicatorBaseScale = activeIndicator->GetScale();
    indicatorPulse = 0.0f;

    // FIXED direction launchYawDeg
    float angleRad = cfg.launchYawDeg * 0.0174532925f;
    float3 dirXZ   = float3(cosf(angleRad), 0.f, sinf(angleRad)).Normalized();

    // Distance to spawn
    std::uniform_real_distribution<float> rad(0.5f * cfg.maxLaunchRadius, cfg.maxLaunchRadius);
    float r           = rad(rng);
    float3 spawnLocal = impactLocal + dirXZ * r;
    spawnLocal.y      = cfg.fallingHeight;
    fireball->SetLocalPosition(spawnLocal);
    fireball->SetEnabled(true);

    // Lateral speed towards impact
    float fallTime   = sqrtf(2.f * cfg.fallingHeight / cfg.gravity);
    float horizSpeed = r / fallTime;
    fireVelocity     = -dirXZ * horizSpeed;
    fireVelocity.y   = 0.f;

    // Shadow
    if (fireballShadow)
    {
        fireballShadow->SetEnabled(true);

        float3 initScale = shadowBaseScale * 0.01f;
        float3 initPos   = float3(spawnLocal.x, 0.f, spawnLocal.z);

        float4x4 tf      = float4x4::FromTRS(initPos, float3x3::identity, initScale);
        fireballShadow->SetLocalTransform(tf);
    }

    dropElapsed     = 0.f;
    activationState = ACTIVATION_STATE::DROPPING;
}

void FireballTrap::HandleImpact()
{
    fireball->SetEnabled(false);
    if (fireballShadow) fireballShadow->SetEnabled(false);

    RecycleGO(activeIndicator);
    activeIndicator = nullptr;

    fireball->SetEnabled(false);
    if (fireballShadow) fireballShadow->SetEnabled(false);

    currentDecal = RequestImpactDecal();
    if (currentDecal) currentDecal->SetLocalPosition(impactOffsetLocal);

    if (groundMesh) groundMesh->SetEnabled(true);
    if (damageCollider) damageCollider->SetEnabled(true);

    float3 playerPos   = character->GetLastPosition();
    float distToPlayer = sqrtf(playerPos.DistanceSq(lastImpactWorld));
    bool hitPlayer     = distToPlayer <= noMiniHitRadius;

    allowMiniDecals    = !hitPlayer;
    SpawnMiniCluster();
    if (shakeCam) shakeCam->StartShake(0.30f, std::clamp(cfg.fallingHeight * 0.03f, 0.15f, 0.6f), 0.12f);

    impactElapsed   = 0.f;
    activationState = ACTIVATION_STATE::DAMAGING;
}

void FireballTrap::DisableDamage()
{
    if (groundMesh) groundMesh->SetEnabled(false);
    if (damageCollider) damageCollider->SetEnabled(false);

    RecycleGO(currentDecal);
    currentDecal    = nullptr;

    activationState = ACTIVATION_STATE::SLEEPING;
}

void FireballTrap::UpdateFireball(float deltaTime)
{
    dropElapsed    += deltaTime;
    fireVelocity.y  = -cfg.gravity * dropElapsed;
    fireVelocity.y  = std::max(fireVelocity.y, -cfg.maxFallSpeed);

    float3 pos      = fireball->GetLocalTransform().TranslatePart();
    pos            += fireVelocity * deltaTime;
    fireball->SetLocalPosition(pos);

    if (fireballShadow)
    {
        float t          = 1.f - std::clamp(pos.y / cfg.fallingHeight, 0.f, 1.f);

        float3 scaleNow  = shadowBaseScale * (0.01f + t * 0.80f);
        float3 shadowPos = float3(pos.x, 0.f, pos.z);

        float4x4 tf      = float4x4::FromTRS(shadowPos, float3x3::identity, scaleNow);
        fireballShadow->SetLocalTransform(tf);
    }

    if (pos.y <= 0.f)
    {
        HandleImpact();
    }
    else
    {
        float4x4 spin =
            float4x4::RotateX(cfg.rotationSpeed * deltaTime) * float4x4::RotateY(cfg.rotationSpeed * deltaTime);
        fireball->SetLocalTransform(fireball->GetLocalTransform() * spin);
    }
}

float FireballTrap::GenerateRandomAttackTime(float min, float max) const
{
    std::uniform_real_distribution<float> dist(min, max);
    return dist(rng);
}

GameObject* FireballTrap::RequestMini()
{
    if (!miniPrototype) return nullptr;

    auto* clone = new GameObject(parent->GetUID(), miniPrototype);
    clone->SetEnabled(true);
    parent->AddChildren(clone->GetUID());
    AppEngine->GetSceneModule()->GetScene()->AddGameObject(clone->GetUID(), clone);
    return clone;
}

void FireballTrap::SpawnMiniCluster()
{
    if (!miniPrototype) return;

    static constexpr float tau = 6.2831853f;
    const float step           = tau / float(miniCount);

    for (uint32_t i = 0; i < miniCount; ++i)
    {
        GameObject* mini = RequestMini();
        if (!mini) continue;

        // Big impact center
        mini->SetLocalPosition(impactOffsetLocal + float3(0.f, 0.5f, 0.f));

        // Horitzontal
        std::uniform_real_distribution<float> jitter(-0.05f, 0.05f);
        float angle  = i * step + jitter(rng);

        float3 dirXZ = float3(cosf(angle), 0.f, sinf(angle)).Normalized();

        // Inital speed horizontal + up speed
        float3 v0;
        const float targetRadius = 0.4f; // meters
        const float startHeight  = 0.5f; // spawn mini height
        const float vUp          = 4.5f; // up speed

        // y<=0:  t = (vUp + sqrt(vUp² + 2*g*startHeight)) / g
        float tFall              = (vUp + sqrtf(vUp * vUp + 2.f * cfg.gravity * startHeight)) / cfg.gravity;

        // horitzontal speed
        float vHoriz             = targetRadius / tFall;

        v0                       = dirXZ * vHoriz;
        v0.y                     = vUp;

        if (auto* col = mini->GetComponent<SphereColliderComponent*>()) col->SetEnabled(true);

        activeMinis.push_back({mini, v0, miniLifeTime});
    }
}

void FireballTrap::UpdateMinis(float deltaTime)
{
    for (auto it = activeMinis.begin(); it != activeMinis.end();)
    {
        it->vel.y  -= cfg.gravity * deltaTime; // g = 9.81
        float3 pos  = it->go->GetLocalTransform().TranslatePart();
        pos        += it->vel * deltaTime;
        it->go->SetLocalPosition(pos);

        bool grounded = (pos.y <= 0.f);

        if (grounded && allowMiniDecals)
        {
            if (GameObject* decal = RequestImpactDecal())
            {
                float3 dPos   = float3(pos.x, 0.f, pos.z);
                float3 dScale = float3(0.4f);
                decal->SetLocalTransform(float4x4::FromTRS(dPos, float3x3::identity, dScale));

                activeMiniDecals.push_back({decal, 1.0f}); // decal lifetime
            }
        }

        // life and ground impact
        it->life     -= deltaTime;
        bool expired  = (it->life <= 0.f) || (pos.y <= 0.f);

        if (expired)
        {
            RecycleGO(it->go);
            it = activeMinis.erase(it);
        }
        else ++it;
    }
}

CameraMovement* FireballTrap::FindShakeCamera()
{
    // find the parent script holding CameraMovement to shake on impact
    CameraComponent* camComp = AppEngine->GetSceneModule()->GetScene()->GetMainCamera();
    if (!camComp) return nullptr;

    GameObject* camGO = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(camComp->GetParentUID());
    if (!camGO) return nullptr;

    GameObject* parentGO = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(camGO->GetParent());
    if (!parentGO) return nullptr;

    ScriptComponent* sc = parentGO->GetComponent<ScriptComponent*>();
    if (!sc) return nullptr;

    return sc->GetScriptByType<CameraMovement>();
}

float3 FireballTrap::RandomSpawnPoint() const
{
    if (!spawnZone) // if no cube = fallback to root position
        return parent->GetGlobalTransform().TranslatePart();

    float3 half   = spawnZone->size * 0.5f; // world‑space half‑extents
    float3 center = parent->GetGlobalTransform().TranslatePart() + spawnZone->centerOffset;

    std::uniform_real_distribution<float> dx(-half.x, half.x);
    std::uniform_real_distribution<float> dz(-half.z, half.z);

    float3 p {center.x + dx(rng), center.y, center.z + dz(rng)};

    // GLOG("Spawn point: %f %f %f", p.x, p.y, p.z);
    return p;
}

GameObject* FireballTrap::RequestImpactDecal()
{
    if (!impactPrefab) return nullptr;

    GameObject* clone = new GameObject(parent->GetUID(), impactPrefab);
    clone->SetEnabled(true);
    parent->AddChildren(clone->GetUID());
    AppEngine->GetSceneModule()->GetScene()->AddGameObject(clone->GetUID(), clone);

    return clone;
}

void FireballTrap::RecycleGO(GameObject* go)
{
    if (!go) return;

    Scene* scene = AppEngine->GetSceneModule()->GetScene();
    scene->RemoveGameObjectHierarchy(go->GetUID());
}

GameObject* FireballTrap::SpawnIndicator(const float3& localPos, float radius)
{
    if (!indicatorPrefab) return nullptr;

    // Clone prefab
    GameObject* ind = new GameObject(parent->GetUID(), indicatorPrefab);

    // Scale, place
    float3 scale    = float3(radius);
    float4x4 tf     = float4x4::FromTRS(localPos, float3x3::identity, scale);
    ind->SetLocalTransform(tf);

    ind->SetEnabled(true);

    // Add to scene
    parent->AddChildren(ind->GetUID());
    AppEngine->GetSceneModule()->GetScene()->AddGameObject(ind->GetUID(), ind);

    return ind;
}
