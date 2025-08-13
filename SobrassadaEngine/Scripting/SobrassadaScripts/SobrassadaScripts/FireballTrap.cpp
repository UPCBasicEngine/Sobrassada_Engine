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
constexpr float SHADOW_MIN_SCALE       = 0.01f;
constexpr float SHADOW_MAX_SCALE       = 0.80f;
constexpr float DEFAULT_DECAL_LIFETIME = 1.0f;

static inline float RandomRange(float min, float max)
{
    static thread_local std::mt19937 rng {std::random_device {}()};
    return std::uniform_real_distribution<float>(min, max)(rng);
}

static inline float3 SafeDiv(const float3& a, const float3& b)
{
    const float eps = 1e-4f;
    return float3(
        a.x / (fabsf(b.x) < eps ? 1.f : b.x), a.y / (fabsf(b.y) < eps ? 1.f : b.y), a.z / (fabsf(b.z) < eps ? 1.f : b.z)
    );
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

    // --- VFX prefabs (assign in Inspector) ---
    fields.push_back({"VFX Main Light", InspectorField::FieldType::GameObject, &vfxMainLightPrefab, 0.f, 0.f});
    fields.push_back({"VFX Light Impact", InspectorField::FieldType::GameObject, &vfxLightImpactPrefab, 0.f, 0.f});
    fields.push_back({"VFX Fire Impact", InspectorField::FieldType::GameObject, &vfxFireImpactPrefab, 0.f, 0.f});
    fields.push_back({"VFX Bomb Ground", InspectorField::FieldType::GameObject, &vfxBombGroundPrefab, 0.f, 0.f});
    fields.push_back({"VFX Black Stain", InspectorField::FieldType::GameObject, &vfxBlackStainPrefab, 0.f, 0.f});

    // --- VFX delays (seconds, relative to IMPACT moment) ---
    fields.push_back({"VFX Delay (Main Light)", InspectorField::FieldType::Float, &vfxMainLightDelay, 0.f, 5.f});
    fields.push_back({"VFX Delay (Light Impact)", InspectorField::FieldType::Float, &vfxLightImpactDelay, 0.f, 5.f});
    fields.push_back({"VFX Delay (Fire Impact)", InspectorField::FieldType::Float, &vfxFireImpactDelay, 0.f, 5.f});
    fields.push_back({"VFX Delay (Bomb Ground)", InspectorField::FieldType::Float, &vfxBombGroundDelay, 0.f, 5.f});
    fields.push_back({"VFX Delay (Black Stain)", InspectorField::FieldType::Float, &vfxBlackStainDelay, 0.f, 5.f});

    // --- VFX lifetimes (seconds) ---
    fields.push_back({"VFX Life (Main Light)", InspectorField::FieldType::Float, &vfxMainLightLife, 0.1f, 10.f});
    fields.push_back({"VFX Life (Light Impact)", InspectorField::FieldType::Float, &vfxLightImpactLife, 0.1f, 10.f});
    fields.push_back({"VFX Life (Fire Impact)", InspectorField::FieldType::Float, &vfxFireImpactLife, 0.1f, 10.f});
    fields.push_back({"VFX Life (Bomb Ground)", InspectorField::FieldType::Float, &vfxBombGroundLife, 0.1f, 10.f});
    fields.push_back({"VFX Life (Black Stain)", InspectorField::FieldType::Float, &vfxBlackStainLife, 0.1f, 10.f});
    fields.push_back({"VFX Indicator (pre-fall)", InspectorField::FieldType::GameObject, &vfxIndicatorPrefab, 0.f, 0.f}
    );
    fields.push_back({"VFX Indicator Scale", InspectorField::FieldType::Float, &vfxIndicatorScale, 0.1f, 5.0f});
    fields.push_back(
        {"VFX Indicator World Radius", InspectorField::FieldType::Float, &vfxIndicatorWorldRadius, 0.05f, 5.0f}
    );

    fields.push_back({"Mini Impact VFX", InspectorField::FieldType::GameObject, &miniImpactVfxPrefab, 0.f, 0.f});
    fields.push_back({"Mini Impact VFX Life", InspectorField::FieldType::Float, &miniImpactVfxLife, 0.1f, 5.0f});
    fields.push_back({"Mini Impact VFX Scale", InspectorField::FieldType::Float, &miniImpactVfxScale, 0.05f, 5.0f});
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

    int idx = 0;
    for (size_t i = 2; i < parent->GetChildren().size() && idx < EXTRA_VFX_COUNT; ++i, ++idx)
    {
        GameObject* child = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(parent->GetChildren()[i]);
        if (!child) continue;

        child->SetEnabled(false);
        extraVfx[idx].go = child;
    }

    if (idx < EXTRA_VFX_COUNT) GLOG("[FireballTrap] Avis: falten %d meshes extres", EXTRA_VFX_COUNT - idx);

    // Deactivate VFX to not view them by default
    if (vfxMainLightPrefab) vfxMainLightPrefab->SetEnabled(false);
    else GLOG("[INFO] VFX Main Light prefab not set");
    if (vfxLightImpactPrefab) vfxLightImpactPrefab->SetEnabled(false);
    else GLOG("[INFO] VFX Light Impact prefab not set");
    if (vfxFireImpactPrefab) vfxFireImpactPrefab->SetEnabled(false);
    else GLOG("[INFO] VFX Fire Impact prefab not set");
    if (vfxBombGroundPrefab) vfxBombGroundPrefab->SetEnabled(false);
    else GLOG("[INFO] VFX Bomb Ground prefab not set");
    if (vfxBlackStainPrefab) vfxBlackStainPrefab->SetEnabled(false);
    else GLOG("[INFO] VFX Black Stain prefab not set (will use legacy impact decal on impact)");
    if (vfxIndicatorPrefab) vfxIndicatorPrefab->SetEnabled(false);
    if (miniImpactVfxPrefab) miniImpactVfxPrefab->SetEnabled(false);

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
    UpdateScheduledVfx(deltaTime);
}

void FireballTrap::StartAttack()
{
    // Reset root transform (trap never moves)
    parent->SetLocalTransform(parent->GetLocalTransform());

    // Random impact position inside spawn zone
    lastImpactWorld     = RandomSpawnPoint();
    impactOffsetLocal   = parent->GetGlobalTransform().Inverted().MulPos(lastImpactWorld);
    impactOffsetLocal.y = 0.f;

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

    // -- Extra VFX 0 (optional mesh, if present)
    if (extraVfx[0].go)
    {
        extraVfx[0].life  = fallTime + 0.1f;
        extraVfx[0].delay = 0.f;
    }

    const float step = 0.05f;
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

    // Reset timeline VFX (existing)
    vfxClock = 0.f;
    for (int i = 0; i < EXTRA_VFX_COUNT; ++i)
    {
        extraVfx[i].timer  = 0.f;
        extraVfx[i].active = false;
        if (extraVfx[i].go) extraVfx[i].go->SetEnabled(false);
    }

    // Schedule impact VFX
    ClearScheduledVfx();
    vfxSchedClock         = 0.f;

    const float impactT   = fallTime;          // moment of impact
    const float3 vfxPos   = impactOffsetLocal; // ground (local to trap)
    const float3 vfxScale = float3::one;

    ScheduleVfx(vfxMainLightPrefab, impactT + vfxMainLightDelay, vfxMainLightLife, vfxPos, vfxScale);
    ScheduleVfx(vfxLightImpactPrefab, impactT + vfxLightImpactDelay, vfxLightImpactLife, vfxPos, vfxScale);
    ScheduleVfx(vfxFireImpactPrefab, impactT + vfxFireImpactDelay, vfxFireImpactLife, vfxPos, vfxScale);
    ScheduleVfx(
        vfxBombGroundPrefab, impactT + vfxBombGroundDelay,
        /*life*/ cfg.bigBurnDuration, vfxPos, vfxScale
    );

    if (vfxBlackStainPrefab)
        ScheduleVfx(vfxBlackStainPrefab, impactT + vfxBlackStainDelay, vfxBlackStainLife, vfxPos, vfxScale);
    if (vfxIndicatorPrefab)
    {
        const float3 indicatorScaleVfx = float3(vfxIndicatorWorldRadius);
        ScheduleVfx(vfxIndicatorPrefab, 0.0f, impactT, vfxPos, indicatorScaleVfx);
    }

    dropElapsed     = 0.f;
    activationState = ACTIVATION_STATE::DROPPING;
}

void FireballTrap::HandleImpact()
{
    // Disable falling visuals
    fireball->SetEnabled(false);
    if (fireballShadow) fireballShadow->SetEnabled(false);

    // Decal under big fireball
    if (!vfxBlackStainPrefab)
    {
        if ((currentDecal = RequestImpactDecal())) currentDecal->SetLocalPosition(impactOffsetLocal);
    }

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
    ClearScheduledVfx();
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

    const float step = TAU / float(miniCount);
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
            float3 dPos = float3(pos.x, 0.f, pos.z);

            if (miniImpactVfxPrefab)
            {
                const float3 localS = SafeDiv(float3(miniImpactVfxScale), parent->GetScale());

                GameObject* vfx     = new GameObject(parent->GetUID(), miniImpactVfxPrefab);
                parent->AddChildren(vfx->GetUID());
                AppEngine->GetSceneModule()->GetScene()->AddGameObject(vfx->GetUID(), vfx);

                const float4x4 tf = float4x4::FromTRS(dPos, float3x3::identity, localS);
                vfx->SetLocalTransform(tf);
                vfx->SetEnabled(true);
                vfx->SetLocalTransform(tf); 

                activeMiniDecals.push_back({vfx, miniImpactVfxLife});
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

void FireballTrap::ScheduleVfx(GameObject* prefab, float delay, float life, const float3& pos, const float3& scale)
{
    if (!prefab) return;
    scheduledVfx.push_back({prefab, delay, life, pos, scale});
}

void FireballTrap::UpdateScheduledVfx(float dt)
{
    vfxSchedClock += dt;

    for (auto& e : scheduledVfx)
    {
        if (!e.prefab) continue;

        // Trigger when delay elapsed
        if (!e.triggered && vfxSchedClock >= e.delay)
        {
            GameObject* inst = new GameObject(parent->GetUID(), e.prefab);

            parent->AddChildren(inst->GetUID());
            AppEngine->GetSceneModule()->GetScene()->AddGameObject(inst->GetUID(), inst);

            const float3 parentS = parent->GetScale();             // escala del pare
            const float3 localS  = SafeDiv(e.localScale, parentS); // local = world / parent

            // TRS local
            const float4x4 tf    = float4x4::FromTRS(e.localPos, float3x3::identity, localS);
            inst->SetLocalTransform(tf);

            inst->SetEnabled(true);
            inst->SetLocalTransform(tf);

            e.instance  = inst;
            e.triggered = true;
            e.timer     = 0.f;
        }

        // Lifetime countdown
        if (e.triggered && e.instance)
        {
            e.timer += dt;
            if (e.timer >= e.life)
            {
                RecycleGO(e.instance);
                e.instance = nullptr; // keep triggered=true to avoid retrigger
            }
        }
    }

    // Purge finished events (triggered and no live instance)
    scheduledVfx.erase(
        std::remove_if(
            scheduledVfx.begin(), scheduledVfx.end(),
            [](const VFXEvent& ev) { return ev.triggered && ev.instance == nullptr; }
        ),
        scheduledVfx.end()
    );
}

void FireballTrap::ClearScheduledVfx()
{
    for (auto& e : scheduledVfx)
    {
        if (e.instance) RecycleGO(e.instance);
        e.instance  = nullptr;
        e.triggered = true;
    }
    scheduledVfx.clear();
    vfxSchedClock = 0.f;
}
