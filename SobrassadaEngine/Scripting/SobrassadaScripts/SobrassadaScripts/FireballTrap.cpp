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
#include "ShaderScriptComponent.h"
#include "Standalone/CharacterControllerComponent.h"
#include "Standalone/MeshComponent.h"
#include "Standalone/Physics/CubeColliderComponent.h"
#include "Standalone/Physics/SphereColliderComponent.h"

constexpr float TAU                  = 2.0f * PI;
constexpr float SHADOW_MIN_SCALE     = 0.01f;
constexpr float SHADOW_MAX_SCALE     = 0.80f;
constexpr float MINI_ANGLE_JITTER    = 0.05f;
constexpr float VFX_CHAIN_STEP       = 0.05f;
constexpr float EXTRA_VFX0_LIFE_EPS  = 0.10f;

constexpr float CAM_SHAKE_DURATION   = 0.30f;
constexpr float CAM_SHAKE_FREQ       = 0.12f;
constexpr float CAM_SHAKE_MAG_FACTOR = 0.03f;
constexpr float CAM_SHAKE_MAG_MIN    = 0.15f;
constexpr float CAM_SHAKE_MAG_MAX    = 0.60f;

constexpr float EPS                  = 1e-4f;

static inline float RandomRange(float min, float max)
{
    static thread_local std::mt19937 rng {std::random_device {}()};
    return std::uniform_real_distribution<float>(min, max)(rng);
}

// Used to convert a world scale into local scale relative to the parent.
static inline float3 SafeDiv(const float3& a, const float3& b)
{
    return float3(
        a.x / (fabsf(b.x) < EPS ? 1.f : b.x), a.y / (fabsf(b.y) < EPS ? 1.f : b.y), a.z / (fabsf(b.z) < EPS ? 1.f : b.z)
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
    fields.push_back(
        {"VFX Indicator World Radius", InspectorField::FieldType::Float, &vfxIndicatorWorldRadius, 0.05f, 5.0f}
    );

    fields.push_back(
        {"Mini Indicator VFX (pre-fall)", InspectorField::FieldType::GameObject, &miniIndicatorVfxPrefab, 0.f, 0.f}
    );
    fields.push_back({"Mini Indicator Scale", InspectorField::FieldType::Float, &miniIndicatorVfxScale, 0.05f, 5.0f});
    fields.push_back({"Mini Start Height", InspectorField::FieldType::Float, &cfg.miniStartHeight, 0.f, 5.f});
    fields.push_back({"Mini Up Speed", InspectorField::FieldType::Float, &cfg.miniUpSpeed, 0.f, 20.f});
    fields.push_back({"Mini Landing Radius", InspectorField::FieldType::Float, &cfg.miniLandingRadius, 0.f, 3.f});

    // Anim prefabs
    fields.push_back({"Anim Wind Name", InspectorField::FieldType::InputText, &animAName});
    fields.push_back({"Anim BigBomb Name", InspectorField::FieldType::InputText, &animBName});
    fields.push_back({"Anim MiniBomb Name", InspectorField::FieldType::InputText, &animCName});

    // Anim delays/life
    fields.push_back({"Anim Wind Delay", InspectorField::FieldType::Float, &animADelay, 0.f, 10.f});
    fields.push_back({"Anim Wind Life", InspectorField::FieldType::Float, &animALife, 0.f, 20.f});

    fields.push_back({"Anim BigBomb Delay", InspectorField::FieldType::Float, &animBDelay, 0.f, 10.f});
    fields.push_back({"Anim BigBomb Life", InspectorField::FieldType::Float, &animBLife, 0.f, 20.f});

    fields.push_back({"Anim MiniBomb Delay", InspectorField::FieldType::Float, &animCDelay, 0.f, 10.f});
    fields.push_back({"Anim MiniBomb Life", InspectorField::FieldType::Float, &animCLife, 0.f, 20.f});
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
    damageAreaCollider = parent->GetComponent<SphereColliderComponent*>();
    if (damageAreaCollider) damageAreaCollider->SetEnabled(false);
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
    if (miniIndicatorVfxPrefab) miniIndicatorVfxPrefab->SetEnabled(false);

    if (!animAPrefab && !animAName.empty())
    {
        animAPrefab = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(animAName);
        if (animAPrefab) animAPrefab->SetEnabled(false);
    }
    if (!animBPrefab && !animBName.empty())
    {
        animBPrefab = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(animBName);
        if (animBPrefab) animBPrefab->SetEnabled(false);
    }
    if (!animCPrefab && !animCName.empty())
    {
        animCPrefab = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(animCName);
        if (animCPrefab) animCPrefab->SetEnabled(false);
    }

    return true;
}

void FireballTrap::Update(float deltaTime)
{
    if (!character || !groundMesh || !damageAreaCollider || !fireball) return;

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

    UpdateScheduledVfx(deltaTime);
    UpdateScheduledAnims(deltaTime);
}

void FireballTrap::StartAttack()
{
    // Random impact position inside spawn zone
    lastImpactWorld            = RandomSpawnPoint();
    impactLocalPos             = parent->GetGlobalTransform().Inverted().MulPos(lastImpactWorld);
    impactLocalPos.y           = 0.f;

    bigBallHitPlayerThisAttack = false;

    // Launch direction
    const float yawRad         = cfg.launchYawDeg * DEGREE_RAD_CONV;
    const float3 dirXZ         = float3(cosf(yawRad), 0.f, sinf(yawRad)).Normalized();

    // Spawn position
    const float launchR        = RandomRange(0.5f * cfg.maxLaunchRadius, cfg.maxLaunchRadius);
    float3 spawnLocal          = impactLocalPos + dirXZ * launchR;
    spawnLocal.y               = cfg.fallingHeight;

    fireball->SetLocalPosition(spawnLocal);
    fireball->SetEnabled(true);

    // Horizontal velocity
    const float fallTime   = sqrtf(2.f * cfg.fallingHeight / cfg.gravity);
    const float horizSpeed = launchR / fallTime;

    fireballVelocity       = -dirXZ * horizSpeed; // towards impact
    if (extraVfx[0].go)
    {
        extraVfx[0].life  = fallTime + EXTRA_VFX0_LIFE_EPS;
        extraVfx[0].delay = 0.f;
    }

    for (int i = 1; i < EXTRA_VFX_COUNT; ++i)
    {
        extraVfx[i].delay = fallTime + (i - 1) * VFX_CHAIN_STEP;
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

    const float impactT   = fallTime;       // moment of impact
    const float3 vfxPos   = impactLocalPos; // ground (local to trap)
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
        if ((currentDecal = RequestImpactDecal())) currentDecal->SetLocalPosition(impactLocalPos);
    }

    // Ground burn mesh & damage collider
    if (groundMesh) groundMesh->SetEnabled(true);
    if (damageAreaCollider)
    {
        damageAreaCollider->SetEnabled(false); // reset
        damageAreaCollider->centerOffset = impactLocalPos;
        damageAreaCollider->SetEnabled(true);
    }

    ScheduleAnim(
        animAPrefab, vfxSchedClock + animADelay, animALife, false, impactLocalPos,
        (animAPrefab ? animAPrefab->GetScale() : float3::one)
    );
    ScheduleAnim(
        animBPrefab, vfxSchedClock + animBDelay, animBLife, false, impactLocalPos,
        (animBPrefab ? animBPrefab->GetScale() : float3::one)
    );
    ScheduleAnim(
        animCPrefab, vfxSchedClock + animCDelay, animCLife, false, impactLocalPos,
        (animCPrefab ? animCPrefab->GetScale() : float3::one)
    );

    if (!bigBallHitPlayerThisAttack)
    {
        plannedMiniAngles.clear();

        const float stepAng     = TAU / float(std::max(1u, miniCount));

        // Physics to compute time-to-ground for minis indicator lives until landing
        const float startHeight = cfg.miniStartHeight;
        const float vUp         = cfg.miniUpSpeed;
        const float g           = cfg.gravity;
        const float tFall       = (vUp + sqrtf(std::max(0.f, vUp * vUp + 2.f * g * startHeight))) / g;

        for (uint32_t i = 0; i < miniCount; ++i)
        {
            const float ang = i * stepAng + RandomRange(-MINI_ANGLE_JITTER, MINI_ANGLE_JITTER);
            plannedMiniAngles.push_back(ang);

            if (miniIndicatorVfxPrefab)
            {
                const float3 dirXZ = float3(cosf(ang), 0.f, sinf(ang));
                const float3 pos   = impactLocalPos + dirXZ * cfg.miniLandingRadius;

                ScheduleVfx(miniIndicatorVfxPrefab, vfxSchedClock, tFall, pos, float3(miniIndicatorVfxScale));
            }
        }

        SpawnMiniCluster();
    }

    if (shakeCam)
    {
        const float cameraShakeMagnitude =
            std::clamp(cfg.fallingHeight * CAM_SHAKE_MAG_FACTOR, CAM_SHAKE_MAG_MIN, CAM_SHAKE_MAG_MAX);
        shakeCam->StartShake(CAM_SHAKE_DURATION, cameraShakeMagnitude, CAM_SHAKE_FREQ);
    }

    impactElapsed   = 0.f;
    activationState = ACTIVATION_STATE::DAMAGING;

    if (extraVfx[0].go) extraVfx[0].go->SetEnabled(false);
    extraVfx[0].active = false;
}

void FireballTrap::DisableDamage()
{
    if (groundMesh) groundMesh->SetEnabled(false);
    if (damageAreaCollider) damageAreaCollider->SetEnabled(false);
    RecycleGO(currentDecal);
    currentDecal    = nullptr;
    activationState = ACTIVATION_STATE::SLEEPING;
    ClearScheduledVfx();
    ClearScheduledAnims();
}

void FireballTrap::UpdateFireball(float deltaTime)
{
    dropElapsed        += deltaTime;
    fireballVelocity.y  = -cfg.gravity * dropElapsed;
    fireballVelocity.y  = std::max(fireballVelocity.y, -cfg.maxFallSpeed);

    float3 pos          = fireball->GetLocalTransform().TranslatePart() + fireballVelocity * deltaTime;
    fireball->SetLocalPosition(pos);

    if (!bigBallHitPlayerThisAttack)
    {
        const float3 fireballWorld = parent->GetGlobalTransform().MulPos(pos);
        const float3 playerWorld   = character->GetLastPosition();

        const float dx             = fireballWorld.x - playerWorld.x;
        const float dz             = fireballWorld.z - playerWorld.z;
        const float distSqXZ       = dx * dx + dz * dz;

        float ballR                = 0.5f;
        if (auto* bc = fireball->GetComponent<SphereColliderComponent*>())
        {
            const float s = std::max(std::max(fireball->GetScale().x, fireball->GetScale().y), fireball->GetScale().z);
            ballR         = bc->radius * s;
        }

        const float playerR = 0.6f;
        const float margin  = 0.2f;
        const float hitR    = ballR + playerR + margin;

        if (distSqXZ <= hitR * hitR)
        {
            bigBallHitPlayerThisAttack = true;

            if (vfxIndicatorPrefab)
            {
                for (auto& e : scheduledVfx)
                {
                    if (e.prefab == vfxIndicatorPrefab && e.instance)
                    {
                        ReleaseToPool(e.instance);
                        e.instance  = nullptr;
                        e.triggered = true;
                    }
                }
            }
        }
    }

    if (fireballShadow)
    {
        const float shadowScaleInterpolation = 1.f - std::clamp(pos.y / cfg.fallingHeight, 0.f, 1.f);
        const float3 scaleNow  = shadowBaseScale * (SHADOW_MIN_SCALE + shadowScaleInterpolation * SHADOW_MAX_SCALE);
        const float3 shadowPos = float3(pos.x, 0.f, pos.z);
        fireballShadow->SetLocalTransform(float4x4::FromTRS(shadowPos, float3x3::identity, scaleNow));
    }

    if (pos.y <= 0.f)
    {
        HandleImpact();
    }
    else
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

    const float stepAng = TAU / float(std::max(1u, miniCount));

    // fallback
    if (plannedMiniAngles.size() != miniCount)
    {
        plannedMiniAngles.clear();
        for (uint32_t i = 0; i < miniCount; ++i)
            plannedMiniAngles.push_back(i * stepAng + RandomRange(-MINI_ANGLE_JITTER, MINI_ANGLE_JITTER));
    }

    const float startHeight = cfg.miniStartHeight;
    const float vUp         = cfg.miniUpSpeed;
    const float g           = cfg.gravity;

    const float tFall       = (vUp + sqrtf(std::max(0.f, vUp * vUp + 2.f * g * startHeight))) / g;

    const float vHoriz      = (tFall > 0.f) ? (cfg.miniLandingRadius / tFall) : 0.f;

    for (uint32_t i = 0; i < miniCount; ++i)
    {
        GameObject* mini = RequestMini();
        if (!mini) continue;

        mini->SetLocalPosition(impactLocalPos + float3(0.f, startHeight, 0.f));

        const float ang  = plannedMiniAngles[i];
        const float3 dir = float3(cosf(ang), 0.f, sinf(ang)).Normalized();

        float3 vel       = dir * vHoriz;
        vel.y            = vUp;

        if (auto* col = mini->GetComponent<SphereColliderComponent*>()) col->SetEnabled(true);
        activeMinis.push_back({mini, vel, miniLifeTime});
    }
}

void FireballTrap::UpdateMinis(float deltaTime)
{
    for (auto it = activeMinis.begin(); it != activeMinis.end();)
    {
        it->velocity.y -= cfg.gravity * deltaTime;
        float3 pos      = it->go->GetLocalTransform().TranslatePart() + it->velocity * deltaTime;
        it->go->SetLocalPosition(pos);

        it->life           -= deltaTime;
        const bool expired  = (it->life <= 0.f) || (pos.y <= 0.f);

        if (expired)
        {
            RecycleGO(it->go);
            it = activeMinis.erase(it);
        }
        else ++it;
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

    float3 half    = spawnZone->size * 0.5f; // world‑space half‑extents
    float3 center  = parent->GetGlobalTransform().TranslatePart() + spawnZone->centerOffset;

    const float rx = RandomRange(-half.x, half.x);
    const float rz = RandomRange(-half.z, half.z);
    float3 p {center.x + rx, center.y, center.z + rz};

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

GameObject* FireballTrap::AcquireFromPool(GameObject* prefab)
{
    if (!prefab) return nullptr;

    auto& pool = vfxPools[prefab];

    for (GameObject* go : pool)
        if (go && !go->IsEnabled()) return go;

    GameObject* inst = CloneHierarchy(prefab, parent->GetUID());
    pool.push_back(inst);
    return inst;
}

void FireballTrap::ReleaseToPool(GameObject* inst)
{
    if (!inst) return;

    if (auto* ssc = inst->GetComponent<ShaderScriptComponent*>()) ssc->SetScriptEnabled("MovingUVClipErode", false);

    StopAnimationsRecursive(inst);
    SetEnabledRecursive(inst, false);
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
            GameObject* inst = AcquireFromPool(e.prefab);
            if (!inst) continue;

            const float3 worldPos = parent->GetGlobalTransform().MulPos(e.localPos);
            const float4x4 tf     = float4x4::FromTRS(worldPos, float3x3::identity, e.localScale);
            inst->SetLocalTransform(tf);
            inst->SetEnabled(true);

            if (auto* ssc = inst->GetComponent<ShaderScriptComponent*>())
            {
                ssc->SetScriptEnabled("MovingUVClipErode", true);
                if (auto* m = inst->GetComponent<MeshComponent*>()) m->SetEnabled(false);
            }

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
                ReleaseToPool(e.instance);
                e.instance = nullptr;
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
        if (e.instance) ReleaseToPool(e.instance);
        e.instance  = nullptr;
        e.triggered = true;
    }
    scheduledVfx.clear();
    vfxSchedClock = 0.f;
}

void FireballTrap::ScheduleAnim(
    GameObject* prefab, float delay, float life, bool loop, const float3& pos, const float3& scale, UID clip
)
{
    if (!prefab) return;
    scheduledAnims.push_back({prefab, clip, delay, life, loop, false, 0.f, nullptr, pos, scale});
}

static inline void StartAnimOnInstance(GameObject* inst, UID clip, bool loop)
{
    if (!inst) return;
    if (auto* ac = inst->GetComponent<AnimationComponent*>())
    {
        if (clip != INVALID_UID) ac->SetAnimationResource(clip);
        ac->OnStop();
        ac->OnPlay(false, loop);
    }
}

void FireballTrap::UpdateScheduledAnims(float dt)
{
    for (auto& e : scheduledAnims)
    {
        if (!e.prefab) continue;

        if (!e.triggered && vfxSchedClock >= e.delay)
        {
            GameObject* inst = AcquireFromPool(e.prefab);
            if (!inst) continue;

            const float4x4 parentW = parent->GetGlobalTransform();
            const float4x4 prefabW = e.prefab->GetGlobalTransform();
            const float4x4 worldTf =
                float4x4::FromTRS(parentW.MulPos(e.localPos), prefabW.RotatePart(), prefabW.ExtractScale());
            const float4x4 localTf = parentW.Inverted() * worldTf;
            inst->SetLocalTransform(localTf);

            SetEnabledRecursive(inst, true);
            StartAnimationsRecursive(inst, e.clip, e.loop);

            if (e.life <= 0.f)
            {
                if (auto* ac = inst->GetComponent<AnimationComponent*>())
                    if (ac->GetCurrentAnimation()) e.life = ac->GetCurrentAnimation()->GetDuration();
                if (e.life <= 0.f) e.life = 1.0f;
            }

            e.instance  = inst;
            e.triggered = true;
            e.timer     = 0.f;
        }

        if (e.triggered && e.instance)
        {
            e.timer += dt;
            if (e.timer >= e.life)
            {
                StopAnimationsRecursive(e.instance);
                ReleaseToPool(e.instance);
                e.instance = nullptr;
            }
        }
    }

    scheduledAnims.erase(
        std::remove_if(
            scheduledAnims.begin(), scheduledAnims.end(),
            [](const AnimEvent& ev) { return ev.triggered && ev.instance == nullptr; }
        ),
        scheduledAnims.end()
    );
}


void FireballTrap::ClearScheduledAnims()
{
    for (auto& e : scheduledAnims)
    {
        if (e.instance)
        {
            StopAnimationsRecursive(e.instance);
            ReleaseToPool(e.instance);
        }
        e.instance  = nullptr;
        e.triggered = true;
    }
    scheduledAnims.clear();
}

GameObject* FireballTrap::CloneHierarchy(GameObject* src, UID newParentUID)
{
    if (!src) return nullptr;

    Scene* sc       = AppEngine->GetSceneModule()->GetScene();

    // Clone this node
    GameObject* dst = new GameObject(newParentUID, src);
    dst->SetEnabled(false);
    sc->AddGameObject(dst->GetUID(), dst);

    // Parent link
    if (newParentUID != INVALID_UID)
    {
        if (GameObject* p = sc->GetGameObjectByUID(newParentUID)) p->AddChildren(dst->GetUID());
    }

    // Recurse on children
    for (UID childUID : src->GetChildren())
    {
        GameObject* srcChild = sc->GetGameObjectByUID(childUID);
        if (!srcChild) continue;
        CloneHierarchy(srcChild, dst->GetUID());
    }

    return dst;
}

void FireballTrap::SetEnabledRecursive(GameObject* go, bool enabled)
{
    if (!go) return;
    go->SetEnabled(enabled);

    Scene* sc = AppEngine->GetSceneModule()->GetScene();
    for (UID cUID : go->GetChildren())
        if (GameObject* c = sc->GetGameObjectByUID(cUID)) SetEnabledRecursive(c, enabled);
}

void FireballTrap::StopAnimationsRecursive(GameObject* go)
{
    if (!go) return;

    if (auto* ac = go->GetComponent<AnimationComponent*>()) ac->OnStop();

    Scene* sc = AppEngine->GetSceneModule()->GetScene();
    for (UID cUID : go->GetChildren())
        if (GameObject* c = sc->GetGameObjectByUID(cUID)) StopAnimationsRecursive(c);
}

void FireballTrap::StartAnimationsRecursive(GameObject* go, UID clip, bool loop)
{
    if (!go) return;

    if (auto* ac = go->GetComponent<AnimationComponent*>())
    {
        if (clip != INVALID_UID) ac->SetAnimationResource(clip);
        ac->OnStop();
        ac->OnPlay(false, loop);
        if (ac->GetAnimationController()) ac->GetAnimationController()->SetTime(0.0f);
        ac->Update(0.0f);
    }

    Scene* sc = AppEngine->GetSceneModule()->GetScene();
    for (UID cUID : go->GetChildren())
        if (GameObject* c = sc->GetGameObjectByUID(cUID)) StartAnimationsRecursive(c, clip, loop);
}
