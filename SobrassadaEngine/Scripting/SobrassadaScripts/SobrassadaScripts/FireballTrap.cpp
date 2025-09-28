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
#include "MiniFireball.h"
#include "ScriptComponent.h"
#include "ShaderScriptComponent.h"
#include "Standalone/Audio/AudioSourceComponent.h"
#include "Standalone/CharacterControllerComponent.h"
#include "Standalone/MeshComponent.h"
#include "Standalone/Physics/CubeColliderComponent.h"
#include "Standalone/Physics/SphereColliderComponent.h"
#include "Wwise_IDs.h"

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

constexpr float SHADOW_Y_BIAS        = 0.015f;

static inline float RandomRange(float min, float max)
{
    static thread_local std::mt19937 rng {std::random_device {}()};
    return std::uniform_real_distribution<float>(min, max)(rng);
}

FireballTrap::FireballTrap(GameObject* parent) : Script(parent)
{
    SetupInspectorFields();
}

FireballTrap::~FireballTrap()
{

    // Clean up cloned VFX
    if (vfxMainLight && vfxMainLight != vfxMainLightPrefab) RecycleGO(vfxMainLight);
    if (vfxLightImpact && vfxLightImpact != vfxLightImpactPrefab) RecycleGO(vfxLightImpact);
    if (vfxFireImpact && vfxFireImpact != vfxFireImpactPrefab) RecycleGO(vfxFireImpact);
    if (vfxBombGround && vfxBombGround != vfxBombGroundPrefab) RecycleGO(vfxBombGround);
    if (vfxBlackStain && vfxBlackStain != vfxBlackStainPrefab) RecycleGO(vfxBlackStain);
    if (vfxIndicator && vfxIndicator != vfxIndicatorPrefab) RecycleGO(vfxIndicator);
    if (vfxBombIndicatorSmallSymbol && vfxBombIndicatorSmallSymbol != vfxBombIndicatorSmallSymbolPrefab)
        RecycleGO(vfxBombIndicatorSmallSymbol);

    for (auto* vfx : miniIndicatorVfx)
    {
        if (vfx && vfx != miniIndicatorVfxPrefab) RecycleGO(vfx);
    }
}

void FireballTrap::SetupInspectorFields()
{
    fields.push_back({"Activation Range", InspectorField::FieldType::Float, &cfg.activationRange, 0.0f, 100.0f});
    fields.push_back({"Min Attack Cooldown", InspectorField::FieldType::Float, &cfg.minAttackCooldown, 0.0f, 10.0f});
    fields.push_back({"Max Attack Cooldown", InspectorField::FieldType::Float, &cfg.maxAttackCooldown, 0.0f, 30.0f});
    fields.push_back({"Trap Damage", InspectorField::FieldType::Int, &cfg.impactDamage, 0, 5});
    fields.push_back({"Damage Duration", InspectorField::FieldType::Float, &cfg.bigBurnDuration, 0.0f, 10.0f});
    fields.push_back({"Rotation Speed", InspectorField::FieldType::Float, &cfg.rotationSpeed, 0.0f, 100.0f});
    fields.push_back({"Falling Height", InspectorField::FieldType::Float, &cfg.fallingHeight, 0.0f, 200.0f});
    fields.push_back({"Max Fall Speed", InspectorField::FieldType::Float, &cfg.maxFallSpeed, 0.0f, 100.0f});
    fields.push_back({"Gravity", InspectorField::FieldType::Float, &cfg.gravity, 0.0f, 20.0f});
    fields.push_back({"Mini Prototype", InspectorField::FieldType::GameObject, &miniPrototype, 0.f, 0.f});
    fields.push_back({"Mini Count", InspectorField::FieldType::Int, &miniCount, 1.f, 12.f});
    fields.push_back({"Mini Lifetime", InspectorField::FieldType::Float, &miniLifeTime, 0.f, 10.f});
    fields.push_back({"Impact Prefab", InspectorField::FieldType::GameObject, &impactPrefab, 0.f, 0.f});
    fields.push_back({"Max Launch Radius", InspectorField::FieldType::Float, &cfg.maxLaunchRadius, 0.f, 20.f});
    fields.push_back({"Direction arc", InspectorField::FieldType::Float, &cfg.launchYawDeg, -180.f, 180.f});

    fields.push_back({"VFX Main Light", InspectorField::FieldType::GameObject, &vfxMainLightPrefab, 0.f, 0.f});
    fields.push_back({"VFX Light Impact", InspectorField::FieldType::GameObject, &vfxLightImpactPrefab, 0.f, 0.f});
    fields.push_back({"VFX Fire Impact", InspectorField::FieldType::GameObject, &vfxFireImpactPrefab, 0.f, 0.f});
    fields.push_back({"VFX Bomb Ground", InspectorField::FieldType::GameObject, &vfxBombGroundPrefab, 0.f, 0.f});
    fields.push_back({"VFX Black Stain", InspectorField::FieldType::GameObject, &vfxBlackStainPrefab, 0.f, 0.f});
    fields.push_back({"VFX Indicator (pre-fall)", InspectorField::FieldType::GameObject, &vfxIndicatorPrefab, 0.f, 0.f}
    );
    fields.push_back(
        {"VFX Indicator World Radius", InspectorField::FieldType::Float, &vfxIndicatorWorldRadius, 0.05f, 5.0f}
    );
    fields.push_back(
        {"Mini Indicator VFX (pre-fall)", InspectorField::FieldType::GameObject, &miniIndicatorVfxPrefab, 0.f, 0.f}
    );
    fields.push_back(
        {"VFX Bomb Indicator Small Symbol", InspectorField::FieldType::GameObject, &vfxBombIndicatorSmallSymbolPrefab,
         0.f, 0.f}
    );
    fields.push_back({"Mini Indicator Scale", InspectorField::FieldType::Float, &miniIndicatorVfxScale, 0.05f, 5.0f});
    fields.push_back({"Mini Start Height", InspectorField::FieldType::Float, &cfg.miniStartHeight, 0.f, 5.f});
    fields.push_back({"Mini Up Speed", InspectorField::FieldType::Float, &cfg.miniUpSpeed, 0.f, 20.f});
    fields.push_back({"Mini Landing Radius", InspectorField::FieldType::Float, &cfg.miniLandingRadius, 0.f, 3.f});

    fields.push_back({"VFX Delay (Main Light)", InspectorField::FieldType::Float, &vfxMainLightDelay, 0.f, 5.f});
    fields.push_back({"VFX Delay (Light Impact)", InspectorField::FieldType::Float, &vfxLightImpactDelay, 0.f, 5.f});
    fields.push_back({"VFX Delay (Fire Impact)", InspectorField::FieldType::Float, &vfxFireImpactDelay, 0.f, 5.f});
    fields.push_back({"VFX Delay (Bomb Ground)", InspectorField::FieldType::Float, &vfxBombGroundDelay, 0.f, 5.f});
    fields.push_back({"VFX Delay (Black Stain)", InspectorField::FieldType::Float, &vfxBlackStainDelay, 0.f, 5.f});

    fields.push_back({"VFX Life (Main Light)", InspectorField::FieldType::Float, &vfxMainLightLife, 0.1f, 10.f});
    fields.push_back({"VFX Life (Light Impact)", InspectorField::FieldType::Float, &vfxLightImpactLife, 0.1f, 10.f});
    fields.push_back({"VFX Life (Fire Impact)", InspectorField::FieldType::Float, &vfxFireImpactLife, 0.1f, 10.f});
    fields.push_back({"VFX Life (Bomb Ground)", InspectorField::FieldType::Float, &vfxBombGroundLife, 0.1f, 10.f});
    fields.push_back({"VFX Life (Black Stain)", InspectorField::FieldType::Float, &vfxBlackStainLife, 0.1f, 10.f});

    fields.push_back({"Anim S Name", InspectorField::FieldType::InputText, &animSName});
    fields.push_back({"Anim N Name", InspectorField::FieldType::InputText, &animNName});
    fields.push_back({"Anim W Name", InspectorField::FieldType::InputText, &animWName});
}

bool FireballTrap::Init()
{
    // Initialize components
    spawnZone = parent->GetComponent<CubeColliderComponent*>();
    if (spawnZone)
    {
        spawnZone->generateCallback = false;
        spawnZone->colliderType     = ColliderType::TRIGGER;
        spawnZone->layer            = ColliderLayer::WORLD_OBJECTS;
        spawnZone->SetEnabled(false);
    }

    groundMesh = parent->GetComponent<MeshComponent*>();
    if (groundMesh) groundMesh->SetEnabled(false);

    damageAreaCollider = parent->GetComponent<SphereColliderComponent*>();
    if (damageAreaCollider) damageAreaCollider->SetEnabled(false);

    // Children setup
    if (!parent->GetChildren().empty())
    {
        fireball = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(parent->GetChildren()[0]);
        if (fireball) fireball->SetEnabled(false);
    }

    if (parent->GetChildren().size() > 1)
    {
        fireballShadow = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(parent->GetChildren()[1]);
        if (fireballShadow)
        {
            fireballShadow->SetEnabled(false);
            shadowBaseScale = fireballShadow->GetScale();
        }
    }

    if (miniPrototype) miniPrototype->SetEnabled(false);
    if (impactPrefab) impactPrefab->SetEnabled(false);

    shakeCam = FindShakeCamera();

    int idx  = 0;
    for (size_t i = 0; i < parent->GetChildren().size(); ++i)
    {
        GameObject* child = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(parent->GetChildren()[i]);
        if (!child) continue;

        // Check for particle system FIRST and skip it from extraVfx
        if (child->GetName() == "VFX_Bomb_Bolts")
        {
            bombNParticleSystem = child->GetComponent<ParticleSystemComponent*>();
            if (!bombNParticleSystem) GLOG("[WARN] Particle system component not found for BombN impact");
            child->SetEnabled(false);
            continue; // SKIP this child, don't add to extraVfx
        }

        if (i >= 8 && idx < EXTRA_VFX_COUNT)
        {
            child->SetEnabled(false);
            extraVfx[idx].go = child;
            idx++;
        }
    }

    // Clone VFX objects from prefabs
    if (vfxMainLightPrefab)
    {
        vfxMainLight = CloneHierarchy(vfxMainLightPrefab, parent->GetUID());
        vfxMainLight->SetEnabled(false);
        auto meshComps = vfxMainLight->GetAllComponentsInChilds<MeshComponent*>(AppEngine);
        for (auto* mc : meshComps)
            mc->SetEnabled(false);
    }

    if (vfxLightImpactPrefab)
    {
        vfxLightImpact = CloneHierarchy(vfxLightImpactPrefab, parent->GetUID());
        vfxLightImpact->SetEnabled(false);
        auto meshComps = vfxLightImpact->GetAllComponentsInChilds<MeshComponent*>(AppEngine);
        for (auto* mc : meshComps)
            mc->SetEnabled(false);
    }

    if (vfxFireImpactPrefab)
    {
        vfxFireImpact = CloneHierarchy(vfxFireImpactPrefab, parent->GetUID());
        vfxFireImpact->SetEnabled(false);
        auto meshComps = vfxFireImpact->GetAllComponentsInChilds<MeshComponent*>(AppEngine);
        for (auto* mc : meshComps)
            mc->SetEnabled(false);
    }

    if (vfxBombGroundPrefab)
    {
        vfxBombGround = CloneHierarchy(vfxBombGroundPrefab, parent->GetUID());
        vfxBombGround->SetEnabled(false);
        auto meshComps = vfxBombGround->GetAllComponentsInChilds<MeshComponent*>(AppEngine);
        for (auto* mc : meshComps)
            mc->SetEnabled(false);
    }

    if (vfxBlackStainPrefab)
    {
        vfxBlackStain = CloneHierarchy(vfxBlackStainPrefab, parent->GetUID());
        vfxBlackStain->SetEnabled(false);
        auto meshComps = vfxBlackStain->GetAllComponentsInChilds<MeshComponent*>(AppEngine);
        for (auto* mc : meshComps)
            mc->SetEnabled(false);
    }

    if (vfxIndicatorPrefab)
    {
        vfxIndicator = CloneHierarchy(vfxIndicatorPrefab, parent->GetUID());
        vfxIndicator->SetEnabled(false);
        vfxIndicatorMeshes = vfxIndicator->GetAllComponentsInChilds<MeshComponent*>(AppEngine);
        GLOG("[INIT] Found %zu meshes in Indicator VFX", vfxIndicatorMeshes.size());
        for (size_t i = 0; i < vfxIndicatorMeshes.size(); ++i)
        {
            auto* mc = vfxIndicatorMeshes[i];
            mc->SetEnabled(false);
            GLOG(
                "[INIT] Indicator Mesh[%zu] disabled, parent: %s", i,
                mc->GetParent() ? mc->GetParent()->GetName().c_str() : "null"
            );
        }
    }

    if (vfxBombIndicatorSmallSymbolPrefab)
    {
        vfxBombIndicatorSmallSymbol = CloneHierarchy(vfxBombIndicatorSmallSymbolPrefab, parent->GetUID());
        vfxBombIndicatorSmallSymbol->SetEnabled(false);

        // Immediately disable mesh components after cloning
        vfxBombIndicatorSmallSymbolMeshes =
            vfxBombIndicatorSmallSymbol->GetAllComponentsInChilds<MeshComponent*>(AppEngine);
        for (auto* mc : vfxBombIndicatorSmallSymbolMeshes)
            mc->SetEnabled(false);
    }

    // Initialize animations
    if (!animSPrefab && !animSName.empty())
    {
        animSPrefab = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(animSName);
        if (animSPrefab)
        {
            animSPrefab->SetEnabledRecursive(false);
            animSPrefab->SetLocalPosition(float3(0, -1000, 0));
        }
    }
    if (!animNPrefab && !animNName.empty())
        animNPrefab = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(animNName);
    if (!animWPrefab && !animWName.empty())
        animWPrefab = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(animWName);

    InitAnimation(animS, animSPrefab, animSName);
    InitAnimation(animN, animNPrefab, animNName);
    InitAnimation(animW, animWPrefab, animWName);

    for (int i = 0; i < MINI_SLOTS; ++i)
    {
        GameObject* go = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(miniSNames[i]);
        if (go)
        {
            go->SetEnabledRecursive(false);
            go->SetLocalPosition(float3(0, -1000, 0));
            InitAnimation(miniS[i], go, miniSNames[i]);
        }
        else
        {
            GLOG("[WARN] Missing mini S anim GO: %s", miniSNames[i].c_str());
        }
    }

    return true;
}

void FireballTrap::EnableVFX(GameObject* vfx, bool enable)
{
    if (!vfx) return;

    if (enable)
    {
        // Enable the GameObject
        vfx->SetEnabled(true);

        // Check if the root GameObject has the specific shader scripts
        if (auto* shaderOnRoot = vfx->GetComponent<ShaderScriptComponent*>())
        {
            bool hasMovingUVShader = false;
            for (const auto& scriptName : shaderOnRoot->GetAllScriptNames())
            {
                if (scriptName == "MovingUVClipErode" || scriptName == "MovingUVTransparent")
                {
                    hasMovingUVShader = true;
                    break;
                }
            }

            // Only disable mesh if it has the moving UV shaders
            if (hasMovingUVShader)
            {
                if (auto* meshOnRoot = vfx->GetComponent<MeshComponent*>())
                {
                    meshOnRoot->SetEnabled(false);
                }
            }

            // Enable the shader scripts
            shaderOnRoot->SetEnabled(true);
            shaderOnRoot->SetScriptEnabled("MovingUVClipErode", true);
            shaderOnRoot->SetScriptEnabled("MovingUVTransparent", true);
        }

        // Only disable mesh if they have the shader scripts
        for (UID childUID : vfx->GetChildren())
        {
            GameObject* child = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(childUID);
            if (!child) continue;

            if (auto* childShader = child->GetComponent<ShaderScriptComponent*>())
            {
                bool hasMovingUVShader = false;
                for (const auto& scriptName : childShader->GetAllScriptNames())
                {
                    if (scriptName == "MovingUVClipErode" || scriptName == "MovingUVTransparent")
                    {
                        hasMovingUVShader = true;
                        break;
                    }
                }

                // Only disable mesh if child has the moving UV shaders
                if (hasMovingUVShader)
                {
                    if (auto* childMesh = child->GetComponent<MeshComponent*>())
                    {
                        childMesh->SetEnabled(false);
                    }
                }

                // Enable shader scripts
                childShader->SetEnabled(true);
                childShader->SetScriptEnabled("MovingUVClipErode", true);
                childShader->SetScriptEnabled("MovingUVTransparent", true);
            }
        }
    }
    else
    {
        vfx->SetEnabled(false);
    }
}

void FireballTrap::ResetVFX(GameObject* vfx)
{
    if (!vfx) return;

    if (auto* rootSSC = vfx->GetComponent<ShaderScriptComponent*>())
    {
        rootSSC->SetScriptEnabled("MovingUVClipErode", false);
        rootSSC->ResetScript("MovingUVClipErode");
        rootSSC->SetScriptEnabled("MovingUVTransparent", false);
        rootSSC->ResetScript("MovingUVTransparent");
    }

    auto shaders = vfx->GetAllComponentsInChilds<ShaderScriptComponent*>(AppEngine);
    for (auto* ssc : shaders)
    {
        ssc->SetScriptEnabled("MovingUVClipErode", false);
        ssc->ResetScript("MovingUVClipErode");
        ssc->SetScriptEnabled("MovingUVTransparent", false);
        ssc->ResetScript("MovingUVTransparent");
    }
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
        if (!v.active && !v.done && vfxClock >= v.delay)
        {
            if (v.go) v.go->SetEnabled(true);
            v.active = true;
            v.timer  = 0.f;
        }
        if (v.active)
        {
            v.timer += deltaTime;
            if (v.timer >= v.life)
            {
                if (v.go) v.go->SetEnabled(false);
                v.active = false;
                v.done   = true;
            }
        }
    }

    UpdateScheduledVfx(deltaTime);
    UpdateAnimation(animS, deltaTime);
    UpdateAnimation(animN, deltaTime);
    UpdateAnimation(animW, deltaTime);

    for (int i = 0; i < MINI_SLOTS; ++i)
        UpdateAnimation(miniS[i], deltaTime);
}

void FireballTrap::StartAttack()
{
    lastImpactWorld            = RandomSpawnPoint();
    impactLocalPos             = parent->GetGlobalTransform().Inverted().MulPos(lastImpactWorld);
    impactLocalPos.y           = 0.f;

    bigBallHitPlayerThisAttack = false;

    const float yawRad         = cfg.launchYawDeg * DEGREE_RAD_CONV;
    const float3 dirXZ         = float3(cosf(yawRad), 0.f, sinf(yawRad)).Normalized();
    const float launchR        = RandomRange(0.5f * cfg.maxLaunchRadius, cfg.maxLaunchRadius);
    float3 spawnLocal          = impactLocalPos + dirXZ * launchR;
    spawnLocal.y               = cfg.fallingHeight;

    fireball->SetLocalPosition(spawnLocal);
    fireball->SetEnabled(true);

    const float fallTime   = sqrtf(2.f * cfg.fallingHeight / cfg.gravity);
    const float horizSpeed = launchR / fallTime;
    fireballVelocity       = -dirXZ * horizSpeed;

    if (extraVfx[0].go)
    {
        extraVfx[0].life  = fallTime + EXTRA_VFX0_LIFE_EPS;
        extraVfx[0].delay = 0.f;
    }
    for (int i = 1; i < EXTRA_VFX_COUNT; ++i)
        extraVfx[i].delay = fallTime + (i - 1) * VFX_CHAIN_STEP;

    if (fireballShadow)
    {
        ResetVFX(fireballShadow);
        fireballShadow->SetEnabled(true);

        if (auto* rootMesh = fireballShadow->GetComponent<MeshComponent*>()) rootMesh->SetEnabled(true);
        for (auto* mc : fireballShadow->GetAllComponentsInChilds<MeshComponent*>(AppEngine))
            mc->SetEnabled(true);

        if (auto* s = fireballShadow->GetComponent<ShaderScriptComponent*>())
        {
            s->SetEnabled(true);
            s->ResetScript("MovingUVTransparent");
            s->SetScriptEnabled("MovingUVTransparent", true);
            s->SetScriptEnabled("MovingUVClipErode", false);
        }
        for (auto* s : fireballShadow->GetAllComponentsInChilds<ShaderScriptComponent*>(AppEngine))
        {
            s->SetEnabled(true);
            s->ResetScript("MovingUVTransparent");
            s->SetScriptEnabled("MovingUVTransparent", true);
            s->SetScriptEnabled("MovingUVClipErode", false);
        }

        const float3 initScale = shadowBaseScale * SHADOW_MIN_SCALE;
        const float3 initPos   = float3(spawnLocal.x, SHADOW_Y_BIAS, spawnLocal.z);
        fireballShadow->SetLocalTransform(float4x4::FromTRS(initPos, float3x3::identity, initScale));
    }

    // Reset timeline VFX
    vfxClock = 0.f;
    for (int i = 0; i < EXTRA_VFX_COUNT; ++i)
    {
        extraVfx[i].timer  = 0.f;
        extraVfx[i].active = false;
        extraVfx[i].done   = false;
        if (extraVfx[i].go) extraVfx[i].go->SetEnabled(false);
    }

    // Schedule impact VFX
    ClearScheduledVfx();
    vfxSchedClock         = 0.f;

    const float impactT   = fallTime;
    const float3 vfxPos   = impactLocalPos;
    const float3 vfxScale = float3::one;
    float3 groundVfxPos   = impactLocalPos;
    groundVfxPos.y        = 0.1f;

    ScheduleVfx(vfxMainLight, impactT + vfxMainLightDelay, vfxMainLightLife, vfxPos, vfxScale);
    ScheduleVfx(vfxLightImpact, impactT + vfxLightImpactDelay, vfxLightImpactLife, vfxPos, vfxScale);
    ScheduleVfx(vfxFireImpact, impactT + vfxFireImpactDelay, vfxFireImpactLife, vfxPos, vfxScale);
    ScheduleVfx(vfxBombGround, impactT + vfxBombGroundDelay, vfxBombGroundLife, groundVfxPos, vfxScale);
    ScheduleVfx(vfxBlackStain, impactT + vfxBlackStainDelay, vfxBlackStainLife, groundVfxPos, vfxScale);

    // Big-ring indicator (pre-fall)
    if (vfxIndicator)
    {
        const float3 indicatorScaleVfx = float3(vfxIndicatorWorldRadius);
        ScheduleVfx(vfxIndicator, 0.0f, impactT, vfxPos, indicatorScaleVfx);
    }

    plannedMiniAngles.clear();
    const float stepAng = TAU / float(std::max(1u, miniCount));
    for (uint32_t i = 0; i < miniCount; ++i)
        plannedMiniAngles.push_back(i * stepAng + RandomRange(-MINI_ANGLE_JITTER, MINI_ANGLE_JITTER));

    const float startHeight = cfg.miniStartHeight;
    const float vUp         = cfg.miniUpSpeed;
    const float g           = cfg.gravity;
    const float miniTFall   = (vUp + sqrtf(std::max(0.f, vUp * vUp + 2.f * g * startHeight))) / g;

    for (auto* v : miniIndicatorVfx)
        if (v && v != miniIndicatorVfxPrefab) RecycleGO(v);
    for (auto* v : miniBombSymbolVfx)
        if (v && v != vfxBombIndicatorSmallSymbolPrefab) RecycleGO(v);
    miniIndicatorVfx.clear();
    miniBombSymbolVfx.clear();

    for (uint32_t i = 0; i < miniCount; ++i)
    {
        const float ang  = plannedMiniAngles[i];
        const float3 dir = float3(cosf(ang), 0.f, sinf(ang));
        const float3 pos = impactLocalPos + dir * cfg.miniLandingRadius;
        const float life = impactT;

        if (miniIndicatorVfxPrefab)
        {
            if (GameObject* miniVfx = CloneHierarchy(miniIndicatorVfxPrefab, parent->GetUID()))
            {
                miniIndicatorVfx.push_back(miniVfx);
                ScheduleVfx(miniVfx, 0.0f, life, pos, float3(miniIndicatorVfxScale * 2.0f));
            }
        }
        if (vfxBombIndicatorSmallSymbolPrefab)
        {
            if (GameObject* symVfx = CloneHierarchy(vfxBombIndicatorSmallSymbolPrefab, parent->GetUID()))
            {
                miniBombSymbolVfx.push_back(symVfx);
                ScheduleVfx(symVfx, 0.0f, life, pos, float3(miniIndicatorVfxScale * 2.0f));
            }
        }
    }

    dropElapsed     = 0.f;
    activationState = ACTIVATION_STATE::DROPPING;
}

void FireballTrap::HandleImpact()
{
    fireball->SetEnabled(false);
    if (fireballShadow) fireballShadow->SetEnabled(false);

    if (groundMesh) groundMesh->SetEnabled(true);
    if (damageAreaCollider)
    {
        damageAreaCollider->SetEnabled(false);
        damageAreaCollider->centerOffset = impactLocalPos;
        damageAreaCollider->SetEnabled(true);
    }

    PlayBombAnimationsAt(impactLocalPos);

    if (!bigBallHitPlayerThisAttack)
    {
        if (auto* audioComp = fireball->GetComponent<AudioSourceComponent*>())
            audioComp->EmitEvent(AK::EVENTS::PLAY_SFX_CATAPULT);
        else if (auto* audioCompP = parent->GetComponent<AudioSourceComponent*>())
            audioCompP->EmitEvent(AK::EVENTS::PLAY_SFX_CATAPULT);

        if (plannedMiniAngles.size() != miniCount)
        {
            plannedMiniAngles.clear();
            const float stepAng = TAU / float(std::max(1u, miniCount));
            for (uint32_t i = 0; i < miniCount; ++i)
                plannedMiniAngles.push_back(i * stepAng + RandomRange(-MINI_ANGLE_JITTER, MINI_ANGLE_JITTER));
        }

        SpawnMiniCluster();
    }
    else
    {
        // If player was hit
        if (vfxIndicator) ResetVFX(vfxIndicator);

        for (auto& e : scheduledVfx)
        {
            bool isMini = false;
            for (auto* mv : miniIndicatorVfx)
                if (e.vfx == mv)
                {
                    isMini = true;
                    break;
                }
            if (!isMini)
                for (auto* sv : miniBombSymbolVfx)
                    if (e.vfx == sv)
                    {
                        isMini = true;
                        break;
                    }

            if (e.vfx == vfxIndicator || e.vfx == vfxBombIndicatorSmallSymbol || isMini)
            {
                EnableVFX(e.vfx, false);
                ResetVFX(e.vfx);
                e.finished  = true;
                e.triggered = false;
            }
        }
    }

    if (shakeCam)
    {
        const float cameraShakeMagnitude =
            std::clamp(cfg.fallingHeight * CAM_SHAKE_MAG_FACTOR, CAM_SHAKE_MAG_MIN, CAM_SHAKE_MAG_MAX);
        shakeCam->StartShake(CAM_SHAKE_DURATION, cameraShakeMagnitude, CAM_SHAKE_FREQ);
    }

    impactElapsed   = 0.f;
    activationState = ACTIVATION_STATE::DAMAGING;

    if (extraVfx[0].go)
    {
        extraVfx[0].go->SetEnabled(false);
        extraVfx[0].active = false;
    }
}

void FireballTrap::DisableDamage()
{
    if (groundMesh) groundMesh->SetEnabled(false);
    if (damageAreaCollider) damageAreaCollider->SetEnabled(false);

    StopBombAnimations();
    for (int i = 0; i < MINI_SLOTS; ++i)
        StopAnimation(miniS[i]);

    RecycleGO(currentDecal);
    currentDecal    = nullptr;
    activationState = ACTIVATION_STATE::SLEEPING;

    // Clean up mini indicators
    for (auto* vfx : miniIndicatorVfx)
    {
        if (vfx && vfx != miniIndicatorVfxPrefab)
        {
            ResetVFX(vfx);
            RecycleGO(vfx);
        }
    }
    miniIndicatorVfx.clear();

    // Clean up mini bomb symbols
    for (auto* vfx : miniBombSymbolVfx)
    {
        if (vfx && vfx != vfxBombIndicatorSmallSymbolPrefab)
        {
            ResetVFX(vfx);
            RecycleGO(vfx);
        }
    }
    miniBombSymbolVfx.clear();
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

            // Hide big indicator immediately
            if (vfxIndicator)
            {
                for (auto& e : scheduledVfx)
                {
                    if (e.vfx == vfxIndicator)
                    {
                        EnableVFX(vfxIndicator, false);
                        ResetVFX(vfxIndicator);
                        e.finished  = true;
                        e.triggered = false;
                    }
                }
            }

            // Hide small symbol on big ring
            if (vfxBombIndicatorSmallSymbol)
            {
                for (auto& e : scheduledVfx)
                {
                    if (e.vfx == vfxBombIndicatorSmallSymbol)
                    {
                        EnableVFX(e.vfx, false);
                        ResetVFX(e.vfx);
                        e.finished  = true;
                        e.triggered = false;
                    }
                }
            }

            for (auto& e : scheduledVfx)
            {
                bool isMini = false;
                for (auto* mv : miniIndicatorVfx)
                    if (e.vfx == mv)
                    {
                        isMini = true;
                        break;
                    }
                if (!isMini)
                    for (auto* sv : miniBombSymbolVfx)
                        if (e.vfx == sv)
                        {
                            isMini = true;
                            break;
                        }

                if (isMini)
                {
                    EnableVFX(e.vfx, false);
                    ResetVFX(e.vfx);
                    e.finished  = true;
                    e.triggered = false;
                }
            }
        }
    }

    if (fireballShadow)
    {
        const float shadowScaleInterpolation = 1.f - std::clamp(pos.y / cfg.fallingHeight, 0.f, 1.f);
        const float3 scaleNow  = shadowBaseScale * (SHADOW_MIN_SCALE + shadowScaleInterpolation * SHADOW_MAX_SCALE);
        const float3 shadowPos = float3(pos.x, SHADOW_Y_BIAS, pos.z);
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
            // Play animation at mini position
            if (pos.y <= 0.f)
            {
                float3 landingPos = pos;
                landingPos.y      = 0.f;
                PlayMiniImpactAnimation(landingPos);
            }

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

void FireballTrap::ScheduleVfx(GameObject* vfx, float delay, float life, const float3& pos, const float3& scale)
{
    if (!vfx) return;

    VFXEvent event;
    event.vfx        = vfx;
    event.delay      = delay;
    event.life       = life;
    event.localPos   = pos;
    event.localScale = scale;
    event.triggered  = false;
    event.timer      = 0.f;
    event.finished   = false;

    event.shaders    = vfx->GetAllComponentsInChilds<ShaderScriptComponent*>(AppEngine);

    scheduledVfx.push_back(event);
}

void FireballTrap::UpdateScheduledVfx(float dt)
{
    vfxSchedClock += dt;

    for (auto& e : scheduledVfx)
    {
        if (!e.vfx || e.finished) continue;

        if (!e.triggered && vfxSchedClock >= e.delay)
        {
            e.triggered            = true;

            // Position the VFX
            const float4x4 parentW = parent->GetGlobalTransform();
            const float4x4 worldTf = float4x4::FromTRS(parentW.MulPos(e.localPos), float3x3::identity, e.localScale);
            const float4x4 localTf = parentW.Inverted() * worldTf;
            e.vfx->SetLocalTransform(localTf);

            // Enable VFX
            EnableVFX(e.vfx, true);
            e.timer = 0.f;
        }

        if (e.triggered)
        {
            e.timer += dt;
            if (e.timer >= e.life)
            {
                EnableVFX(e.vfx, false);
                ResetVFX(e.vfx);
                auto meshComps = e.vfx->GetAllComponentsInChilds<MeshComponent*>(AppEngine);
                for (auto* mc : meshComps)
                    mc->SetEnabled(false);

                e.triggered = false;
                e.timer     = 0.f;
                e.finished  = true;
            }
        }
    }
    scheduledVfx.erase(
        std::remove_if(scheduledVfx.begin(), scheduledVfx.end(), [](const VFXEvent& e) { return e.finished; }),
        scheduledVfx.end()
    );
}

void FireballTrap::ClearScheduledVfx()
{
    for (auto& e : scheduledVfx)
    {
        if (e.vfx)
        {
            EnableVFX(e.vfx, false);
            ResetVFX(e.vfx);
            auto meshComps = e.vfx->GetAllComponentsInChilds<MeshComponent*>(AppEngine);
            for (auto* mc : meshComps)
                mc->SetEnabled(false);
        }
    }
    scheduledVfx.clear();
    vfxSchedClock = 0.f;
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

        // Set random initial rotation for visual variety
        if (auto* sc = mini->GetComponent<ScriptComponent*>())
        {
            if (auto* miniScript = sc->GetScriptByType<MiniFireball>())
            {
                // Random initial rotation
                float3 randomRot = float3(RandomRange(0.f, 360.f), RandomRange(0.f, 360.f), RandomRange(0.f, 360.f));
                miniScript->SetInitialRotation(randomRot);

                // Random rotation speed
                float3 randomSpeed = float3(
                    RandomRange(60.f, 150.f), // X rotation speed
                    RandomRange(90.f, 180.f), // Y rotation speed
                    RandomRange(30.f, 90.f)   // Z rotation speed
                );
                miniScript->SetRotationSpeed(randomSpeed);
            }
        }

        if (auto* col = mini->GetComponent<SphereColliderComponent*>()) col->SetEnabled(true);

        activeMinis.push_back({mini, vel, miniLifeTime});
    }
}

GameObject* FireballTrap::CloneHierarchy(GameObject* src, UID newParentUID)
{
    if (!src) return nullptr;

    Scene* sc       = AppEngine->GetSceneModule()->GetScene();

    GameObject* dst = new GameObject(newParentUID, src);
    dst->SetEnabled(false);
    sc->AddGameObject(dst->GetUID(), dst);

    if (newParentUID != INVALID_UID)
    {
        if (GameObject* p = sc->GetGameObjectByUID(newParentUID)) p->AddChildren(dst->GetUID());
    }

    for (UID childUID : src->GetChildren())
    {
        GameObject* srcChild = sc->GetGameObjectByUID(childUID);
        if (!srcChild) continue;
        CloneHierarchy(srcChild, dst->GetUID());
    }

    return dst;
}

void FireballTrap::RecycleGO(GameObject* go) const
{
    if (!go) return;
    Scene* scene = AppEngine->GetSceneModule()->GetScene();
    scene->QueueGameObjectDelete(go->GetUID());
}

void FireballTrap::StopAnimationsRecursive(GameObject* go)
{
    if (!go) return;

    // Stop all animations in hierarchy
    auto allAnims = go->GetAllComponentsInChilds<AnimationComponent*>(AppEngine);
    for (auto* ac : allAnims)
    {
        if (ac) ac->OnStop();
    }
}

void FireballTrap::PlayBombAnimationsAt(const float3& localPos)
{
    PlayAnimationAt(animN, localPos);
    PlayAnimationAt(animW, localPos);
}

void FireballTrap::StopBombAnimations()
{
    StopAnimation(animS);
    StopAnimation(animN);
    StopAnimation(animW);
}

void FireballTrap::StopAnimation(OneShotAnim& anim)
{
    if (!anim.root) return;

    if (anim.ac) anim.ac->OnStop();
    anim.root->SetEnabledRecursive(false);
    anim.root->SetLocalPosition(float3(0, -1000, 0));
    anim.playing = false;
}

bool FireballTrap::InitAnimation(OneShotAnim& anim, GameObject* prefab, const std::string& name)
{
    if (!prefab) return false;

    anim.root = prefab;

    // Find the animation component
    anim.ac   = prefab->GetComponent<AnimationComponent*>();

    if (!anim.ac)
    {
        auto v = prefab->GetAllComponentsInChilds<AnimationComponent*>(AppEngine);
        if (!v.empty())
        {
            anim.ac = v.front();
            GLOG("[INFO] Found animation component in child for %s", name.c_str());
        }
    }

    if (anim.ac)
    {
        GLOG("[INFO] Animation component ready for %s, resource: %llu", name.c_str(), anim.ac->GetAnimationResource());
        if (!anim.ac->GetCurrentAnimation())
        {
            anim.ac->Init();
            anim.ac->OnStop();
        }
    }
    else
    {
        GLOG("[WARNING] No animation component found for %s", name.c_str());
    }

    prefab->SetEnabledRecursive(false);
    anim.playing = false;

    return true;
}

void FireballTrap::UpdateAnimation(OneShotAnim& anim, float deltaTime)
{
    if (anim.playing && anim.ac)
    {
        anim.ac->Update(deltaTime);
        if (anim.ac->IsFinished())
        {
            if (&anim == &animN && bombNParticleSystem)
            {
                GameObject* psGO =
                    AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(bombNParticleSystem->GetParentUID());
                if (psGO && anim.root)
                {
                    // Enable the GameObject first!
                    psGO->SetEnabled(true);

                    // Position at animation's location
                    float3 spawnPos  = anim.root->GetLocalTransform().TranslatePart();
                    spawnPos.y      += 1.0f; // Adjust height as needed
                    psGO->SetLocalPosition(spawnPos);

                    // Now spawn the particles
                    bombNParticleSystem->SpawnAllInstances();
                }
            }

            StopAnimation(anim);
        }
    }
}

void FireballTrap::PlayAnimationAt(OneShotAnim& anim, const float3& localPos)
{
    if (!anim.root || !anim.ac) return;

    if (anim.root->GetParent() != parent->GetUID())
    {
        anim.root->SetParent(parent->GetUID());
        parent->AddChildren(anim.root->GetUID());
    }

    anim.root->SetLocalPosition(localPos);
    anim.root->SetEnabledRecursive(true);

    // Play animation
    anim.ac->SetBoneMapping();
    anim.ac->OnStop();
    anim.ac->OnPlay(false, false);

    if (auto* ctrl = anim.ac->GetAnimationController())
    {
        ctrl->SetTime(0.0f);

        // Slow down animW specifically
        if (&anim == &animW)
        {
            ctrl->SetPlaybackSpeed(0.2f); // Half speed = double duration
        }
        else
        {
            ctrl->SetPlaybackSpeed(1.0f); // Normal speed for others
        }
    }

    anim.ac->Update(0.001f);
    anim.playing = true;
}

// Play animation at mini impact location
void FireballTrap::PlayMiniImpactAnimation(const float3& localPos)
{
    OneShotAnim& slot = miniS[miniSNext];
    miniSNext         = (miniSNext + 1) % MINI_SLOTS;

    if (!slot.root || !slot.ac) return;

    if (slot.playing) StopAnimation(slot); // reuse if still busy

    float3 p = localPos;
    p.y      = std::max(0.02f, p.y);
    PlayAnimationAt(slot, p);
}

void FireballTrap::OnPlayerExitLocation()
{
    // Reset all active VFX when player leaves the area
    if (vfxMainLight) ResetVFX(vfxMainLight);
    if (vfxLightImpact) ResetVFX(vfxLightImpact);
    if (vfxFireImpact) ResetVFX(vfxFireImpact);
    if (vfxBombGround) ResetVFX(vfxBombGround);
    if (vfxBlackStain) ResetVFX(vfxBlackStain);
    if (vfxIndicator) ResetVFX(vfxIndicator);
    if (vfxBombIndicatorSmallSymbol) ResetVFX(vfxBombIndicatorSmallSymbol);

    for (auto* vfx : miniIndicatorVfx)
    {
        if (vfx && vfx != miniIndicatorVfxPrefab) ResetVFX(vfx);
    }

    // Reset trap state
    activationState = ACTIVATION_STATE::SLEEPING;
    ClearScheduledVfx();
}