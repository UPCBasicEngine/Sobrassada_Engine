#include "pch.h"
#undef max
#undef min

#include <algorithm>
#include <random>

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

constexpr float TAU                    = 2.0f * PI;
constexpr float INDICATOR_PULSE_SPEED  = 4.0f;
constexpr float INDICATOR_PULSE_SCALE  = 0.25f;
constexpr float SHADOW_MIN_SCALE       = 0.01f;
constexpr float SHADOW_MAX_SCALE       = 0.80f;
constexpr float DEFAULT_DECAL_LIFETIME = 1.0f;

static inline float RandomRange(float min, float max)
{
    static thread_local std::mt19937 rng {std::random_device {}()};
    return std::uniform_real_distribution<float>(min, max)(rng);
}

FireballTrap::FireballTrap(GameObject* parent) : Script(parent)
{
    SetupInspectorFields();
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
    fields.push_back({"Mini Lifetime", InspectorField::FieldType::Float, &miniLifeTime, 0.f, 10.f});

    // Impact decals
    fields.push_back({"Impact Prefab", InspectorField::FieldType::GameObject, &impactPrefab, 0.f, 0.f});

    // Arc
    fields.push_back({"Max Launch Radius", InspectorField::FieldType::Float, &cfg.maxLaunchRadius, 0.f, 20.f});
    fields.push_back({"Direction arc", InspectorField::FieldType::Float, &cfg.launchYawDeg, -180.f, 180.f});

    // Fall indicator
    fields.push_back({"Landing Indicator", InspectorField::FieldType::GameObject, &indicatorPrefab, 0.f, 0.f});
    fields.push_back({"Indicator Scale", InspectorField::FieldType::Float, &indicatorScale, 0.1f, 2.0f});

    fields.push_back({"Life 1", InspectorField::FieldType::Float, &extraVfx[1].life, 0.1f, 10.f});
}
bool FireballTrap::Init()
{
    // Collider: spawn zone
    spawnZone = parent->GetComponent<CubeColliderComponent*>();
    if (spawnZone)
    {
        spawnZone->generateCallback = false;
        spawnZone->colliderType     = ColliderType::TRIGGER;
        spawnZone->layer            = ColliderLayer::WORLD_OBJECTS;
        spawnZone->SetEnabled(false);
    }
    else GLOG("[WARNING] FireballTrap: Spawn zone CubeCollider not found");

    // Visual ground burn mesh
    groundMesh = parent->GetComponent<MeshComponent*>();
    if (groundMesh) groundMesh->SetEnabled(false);
    else GLOG("[WARNING] FireballTrap without mesh component.");

    // Damage collider
    damageCollider = parent->GetComponent<SphereColliderComponent*>();
    if (damageCollider) damageCollider->SetEnabled(false);
    else GLOG("[WARNING] FireballTrap without sphere collider component.");

    // Children: 0 = fireball, 1 = shadow
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
        shadowBaseScale = fireballShadow ? fireballShadow->GetScale() : float3::one;
    }

    if (miniPrototype) miniPrototype->SetEnabled(false);
    else GLOG("[WARNING] FireballTrap: Mini prototype reference not set");

    if (impactPrefab) impactPrefab->SetEnabled(false);
    else GLOG("[WARNING] FireballTrap: Impact prefab reference not set");

    shakeCam = FindShakeCamera();
    if (!shakeCam) GLOG("[WARNING] FireballTrap: CameraMovement not found");

    if (indicatorPrefab) indicatorPrefab->SetEnabled(false);
    else GLOG("[WARNING] FireballTrap: landing indicator prefab not set");

    int idx = 0;
    for (size_t i = 2; i < parent->GetChildren().size() && idx < EXTRA_VFX_COUNT; ++i, ++idx)
    {
        GameObject* child = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(parent->GetChildren()[i]);
        if (!child) continue;

        child->SetEnabled(false);
        extraVfx[idx].go = child;
    }

    if (idx < EXTRA_VFX_COUNT) GLOG("[FireballTrap] Avis: falten %d meshes extres", EXTRA_VFX_COUNT - idx);

    return true;
}

void FireballTrap::Update(float deltaTime)
{
    if (!character || !groundMesh || !damageCollider || !fireball) return;

    switch (activationState)
    {
    case ACTIVATION_STATE::SLEEPING:
    {
        const float distanceSquaredToPlayer =
            character->GetLastPosition().DistanceSq(parent->GetGlobalTransform().TranslatePart());
        if (distanceSquaredToPlayer <= cfg.activationRange * cfg.activationRange)
        {
            randomAttackTime = GenerateRandomAttackTime(cfg.minAttackCooldown, cfg.maxAttackCooldown);
            activatedTime    = 0.f;
            activationState  = ACTIVATION_STATE::IDLE;
        }
        break;
    }
    case ACTIVATION_STATE::IDLE:
        if ((activatedTime += deltaTime) >= randomAttackTime) StartAttack();
        break;

    case ACTIVATION_STATE::DROPPING:
        UpdateFireball(deltaTime);
        break;

    case ACTIVATION_STATE::DAMAGING:
        if ((impactElapsed += deltaTime) >= cfg.bigBurnDuration) DisableDamage();
        break;
    }

    UpdateMinis(deltaTime);

    vfxClock += deltaTime;

    for (int i = 0; i < EXTRA_VFX_COUNT; ++i)
    {
        TimedVFX& v = extraVfx[i];
        if (!v.active && vfxClock >= v.delay)
        {
            if (v.go) v.go->SetEnabled(true);
            v.active = true;
        }
        if (v.active)
        {
            v.timer += deltaTime;
            if (v.timer >= v.life)
            {
                if (v.go) v.go->SetEnabled(false);
                v.active = false;
            }
        }
    }

    // Landing indicator pulse animation
    if (activeIndicator)
    {
        indicatorPulse               += deltaTime * INDICATOR_PULSE_SPEED;
        const float pulseScaleFactor  = 1.0f + INDICATOR_PULSE_SCALE * sinf(indicatorPulse);

        const float3 pos              = activeIndicator->GetLocalTransform().TranslatePart();
        const float3 scale            = indicatorBaseScale * pulseScaleFactor;

        activeIndicator->SetLocalTransform(float4x4::FromTRS(pos, float3x3::identity, scale));
    }

    // Lifetime & cleanup of mini decals
    for (auto it = activeMiniDecals.begin(); it != activeMiniDecals.end();)
    {
        if ((it->timer -= deltaTime) <= 0.f)
        {
            RecycleGO(it->go);
            it = activeMiniDecals.erase(it);
        }
        else ++it;
    }
}

void FireballTrap::StartAttack()
{
    // Reset root transform (trap never moves)
    parent->SetLocalTransform(parent->GetLocalTransform());

    // Random impact position inside spawn zone
    lastImpactWorld     = RandomSpawnPoint();
    impactOffsetLocal   = parent->GetGlobalTransform().Inverted().MulPos(lastImpactWorld);
    impactOffsetLocal.y = 0.f;

    // Visual landing indicator
    if (activeIndicator) RecycleGO(activeIndicator);

    activeIndicator = SpawnIndicator(impactOffsetLocal, cfg.bigBurnRadius * indicatorScale);
    if (activeIndicator) indicatorBaseScale = activeIndicator->GetScale();
    indicatorPulse      = 0.0f;

    // Launch direction
    const float yawRad  = cfg.launchYawDeg * DEGREE_RAD_CONV;
    const float3 dirXZ  = float3(cosf(yawRad), 0.f, sinf(yawRad)).Normalized();

    // Spawn position
    const float launchR = RandomRange(0.5f * cfg.maxLaunchRadius, cfg.maxLaunchRadius);
    float3 spawnLocal   = impactOffsetLocal + dirXZ * launchR;
    spawnLocal.y        = cfg.fallingHeight;

    fireball->SetLocalPosition(spawnLocal);
    fireball->SetEnabled(true);

    // Horizontal velocity
    const float fallTime   = sqrtf(2.f * cfg.fallingHeight / cfg.gravity);
    const float horizSpeed = launchR / fallTime;

    fireVelocity           = -dirXZ * horizSpeed; // towards impact

    // -- VFX 0 (circle)
    extraVfx[0].life       = fallTime + 0.1f; 
    extraVfx[0].delay      = 0.f;             

    const float step       = 0.05f;
    for (int i = 1; i < EXTRA_VFX_COUNT; ++i)
    {
        extraVfx[i].delay = fallTime + (i - 1) * step;
    }

    // Shadow initial placement
    if (fireballShadow)
    {
        fireballShadow->SetEnabled(true);

        const float3 initScale = shadowBaseScale * SHADOW_MIN_SCALE;
        const float3 initPos   = float3(spawnLocal.x, 0.f, spawnLocal.z);

        fireballShadow->SetLocalTransform(float4x4::FromTRS(initPos, float3x3::identity, initScale));
    }

    // Reset timeline VFX
    vfxClock = 0.f;
    for (int i = 0; i < EXTRA_VFX_COUNT; ++i)
    {
        extraVfx[i].timer  = 0.f;
        extraVfx[i].active = false;
        if (extraVfx[i].go) extraVfx[i].go->SetEnabled(false);
    }

    dropElapsed     = 0.f;
    activationState = ACTIVATION_STATE::DROPPING;
}

void FireballTrap::HandleImpact()
{
    // Disable falling visuals
    fireball->SetEnabled(false);
    if (fireballShadow) fireballShadow->SetEnabled(false);

    RecycleGO(activeIndicator);
    activeIndicator = nullptr;

    // Decal under big fireball
    if ((currentDecal = RequestImpactDecal())) currentDecal->SetLocalPosition(impactOffsetLocal);

    // Ground burn mesh & damage collider
    if (groundMesh) groundMesh->SetEnabled(true);
    if (damageCollider)
    {
        damageCollider->SetEnabled(false); // reset
        damageCollider->centerOffset = impactOffsetLocal;
        damageCollider->SetEnabled(true);
    }

    // Spawn minis
    const float distToPlayer = sqrtf(character->GetLastPosition().DistanceSq(lastImpactWorld));
    allowMiniDecals          = (distToPlayer > noMiniHitRadius);

    SpawnMiniCluster();

    if (shakeCam)
    {
        const float cameraShakeMagnitude = std::clamp(cfg.fallingHeight * 0.03f, 0.15f, 0.6f);
        shakeCam->StartShake(0.30f, cameraShakeMagnitude, 0.12f);
    }

    impactElapsed   = 0.f;
    activationState = ACTIVATION_STATE::DAMAGING;

    if (extraVfx[0].go) extraVfx[0].go->SetEnabled(false);
    extraVfx[0].active = false;
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

    float3 pos      = fireball->GetLocalTransform().TranslatePart() + fireVelocity * deltaTime;
    fireball->SetLocalPosition(pos);

    // Shadow scaling
    if (fireballShadow)
    {
        const float shadowScaleInterpolation = 1.f - std::clamp(pos.y / cfg.fallingHeight, 0.f, 1.f);
        const float3 scaleNow  = shadowBaseScale * (SHADOW_MIN_SCALE + shadowScaleInterpolation * SHADOW_MAX_SCALE);
        const float3 shadowPos = float3(pos.x, 0.f, pos.z);
        fireballShadow->SetLocalTransform(float4x4::FromTRS(shadowPos, float3x3::identity, scaleNow));
    }

    // Impact
    if (pos.y <= 0.f)
    {
        HandleImpact();
    }
    else // spin while falling
    {
        const float4x4 spin =
            float4x4::RotateX(cfg.rotationSpeed * deltaTime) * float4x4::RotateY(cfg.rotationSpeed * deltaTime);
        fireball->SetLocalTransform(fireball->GetLocalTransform() * spin);
    }
}

float FireballTrap::GenerateRandomAttackTime(float min, float max) const
{
    return RandomRange(min, max);
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

    static constexpr float tau = 2.f * PI;
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
        float3 miniInitialVelocity;
        const float targetRadius  = 0.4f; // meters
        const float startHeight   = 0.5f; // spawn mini height
        const float vUp           = 4.5f; // up speed

        // y<=0:  t = (vUp + sqrt(vUp² + 2*g*startHeight)) / g
        float miniFallDuration    = (vUp + sqrtf(vUp * vUp + 2.f * cfg.gravity * startHeight)) / cfg.gravity;

        // horitzontal speed
        float miniHorizontalSpeed = targetRadius / miniFallDuration;

        miniInitialVelocity       = dirXZ * miniHorizontalSpeed;
        miniInitialVelocity.y     = vUp;

        if (auto* col = mini->GetComponent<SphereColliderComponent*>()) col->SetEnabled(true);

        activeMinis.push_back({mini, miniInitialVelocity, miniLifeTime});
    }
}

void FireballTrap::UpdateMinis(float deltaTime)
{
    for (auto miniProjectile = activeMinis.begin(); miniProjectile != activeMinis.end();)
    {
        miniProjectile->vel.y -= cfg.gravity * deltaTime; // g = 9.81
        float3 pos             = miniProjectile->go->GetLocalTransform().TranslatePart();
        pos                   += miniProjectile->vel * deltaTime;
        miniProjectile->go->SetLocalPosition(pos);

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
        miniProjectile->life -= deltaTime;
        bool expired          = (miniProjectile->life <= 0.f) || (pos.y <= 0.f);

        if (expired)
        {
            RecycleGO(miniProjectile->go);
            miniProjectile = activeMinis.erase(miniProjectile);
        }
        else ++miniProjectile;
    }
}

CameraMovement* FireballTrap::FindShakeCamera() const
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

void FireballTrap::RecycleGO(GameObject* go) const
{
    if (!go) return;
    Scene* scene = AppEngine->GetSceneModule()->GetScene();
    scene->QueueGameObjectDelete(go->GetUID());
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
