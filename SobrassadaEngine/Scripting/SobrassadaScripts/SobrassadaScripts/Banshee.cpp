#include "pch.h"

#include "Banshee.h"

#include "CuChulainn.h"
#include "DebugDrawModule.h"
#include "GameObject.h"
#include "GameTimer.h"
#include "Interpolation.h"
#include "ParticleSystemComponent.h"
#include "ResourceMaterial.h"
#include "ResourcesModule.h"
#include "ShaderScriptComponent.h"
#include "Standalone/AIAgentComponent.h"
#include "Standalone/AnimationComponent.h"
#include "Standalone/CharacterControllerComponent.h"
#include "Standalone/MeshComponent.h"
#include "Standalone/Physics/CapsuleColliderComponent.h"
#include "Standalone/Physics/CubeColliderComponent.h"
#include "Standalone/Physics/SphereColliderComponent.h"

#include "imgui_curve_editor.h"

Banshee::Banshee(GameObject* parent)
    : Character(
          parent,
          2,     // Max Health
          2,     // Damage
          2.0f,  // Attack Duration
          4.0f,  // Attack Cooldown
          5.0f,  // Attack Range
          5.0f,  // AI Aggro Range
          5.0f,  // AI Chase Range
          10.0f, // Max detection range
          CharacterType::Banshee
      )
{
    fields.push_back({InspectorField::FieldType::Text, (void*)"Invisibility parameters"});
    fields.push_back({"Invisible time range", InspectorField::FieldType::Vec2, &invisibleTimeRange, 0.0f, 10.0f});
    fields.push_back({"Teleport VFX duration", InspectorField::FieldType::Float, &teleportVFXDuration, 0.1f, 10.0f});

    fields.push_back({InspectorField::FieldType::Text, (void*)"Scream parameters"});
    fields.push_back({"Attack Angular Speed", InspectorField::FieldType::Float, &attackAngularSpeed, 0.0f, 10.0f});
    fields.push_back({"Main scream duration", InspectorField::FieldType::Float, &mainScreamDuration, 0.1f, 10.0f});
    fields.push_back({"Warning duration", InspectorField::FieldType::Float, &warningDuration, 0.1f, 10.0f});

    fields.push_back({InspectorField::FieldType::Text, (void*)"Slow area parameters"});
    fields.push_back({"Slow Area Damage", InspectorField::FieldType::Int, &slowAreaDamage, 0, 10});
    fields.push_back(
        {"Slow Area Warning Duration", InspectorField::FieldType::Float, &slowAreaWaringDuration, 0.f, 10.f}
    );
    fields.push_back(
        {"Slow Area Warning Max Scale", InspectorField::FieldType::Float, &slowAreaWaringMaxScale, 0.f, 10.f}
    );
    fields.push_back({"Slow area duration", InspectorField::FieldType::Float, &slowAreaDuration, 0.f, 10.f});

    fields.push_back({InspectorField::FieldType::Text, (void*)"Teleport warning size curve"});
    for (int i = 0; i < maxScriptCurvePoints; ++i)
    {
        curveEditorPoints[i].x = (float)i / 10.f;
        curveEditorPoints[i].y = (float)i / 10.f;
    }

    curveEditorPoints[maxScriptCurvePoints].x = 0.f;
    curveEditorPoints[maxScriptCurvePoints].y = 1.f;

    curveEditorPoints[0].x                    = ImGui::CurveTerminator;

    fields.push_back({"Teleport warning Size", InspectorField::FieldType::CurveEditor, &curveEditorPoints, 0.f, 10.f});
}

bool Banshee::Init()
{
    Character::Init();

    agentAI = parent->GetComponent<AIAgentComponent*>();
    if (agentAI == nullptr) GLOG("AIAgent component not found for Banshee")
    else
    {
        agentAI->RecreateAgent();
        agentAI->SetLookForward(true);
        speed = agentAI->GetSpeed();
    }

    const std::vector<UID>& childVector = parent->GetChildren();
    Scene* loadedScene                  = AppEngine->GetSceneModule()->GetScene();

    if (!loadedScene)
    {
        GLOG("[ERROR BANSHEE SCRIPT]: Scene pointer nullptr")
        return true;
    }

    for (UID childUID : childVector)
    {
        GameObject* currentGO = loadedScene->GetGameObjectByUID(childUID);

        if (currentGO->GetName() == "BansheeMesh")
        {
            mesh = currentGO->GetComponent<MeshComponent*>();
            if (!mesh) GLOG("[ERROR BANSHEE SCRIPT]: No mesh found")
        }
        else if (currentGO->GetName() == "Scream")
        {
            weapon = currentGO;

            weapon->SetEnabled(false);
        }
        else if (currentGO->GetName() == "SlowArea")
        {
            slowAreaGO          = currentGO;
            slowAreaStartHeight = slowAreaGO->GetLocalTransform().TranslatePart().y;
            slowAreaGO->SetEnabled(false);
        }
        else if (currentGO->GetName() == "SlowAreaIn")
        {
            slowAreaInGO          = currentGO;
            slowAreaInStartHeight = slowAreaInGO->GetLocalTransform().TranslatePart().y;
            slowAreaInGO->SetEnabled(false);
        }
        else if (currentGO->GetName() == "SlowAreaWarning")
        {
            slowAreaWarningGO = currentGO;

            float3 translation, scale;
            Quat rotation;

            slowAreaWarningGO->GetLocalTransform().Decompose(translation, rotation, scale);
            float4x4 starTransform =
                float4x4::FromTRS(translation, rotation, float3(slowAreaWaringMaxScale, 1.f, slowAreaWaringMaxScale));
            slowAreaWarningGO->SetLocalTransform(starTransform);
            slowAreaWarningGO->SetEnabled(false);

            slowWarningStartHeight = translation.y;
        }
        else if (currentGO->GetName() == "TeleportWarning")
        {
            teleportWarningScreamGO = currentGO;
            teleportWarningScreamGO->SetEnabled(false);

            MeshComponent* mesh = teleportWarningScreamGO->GetComponent<MeshComponent*>();
            if (mesh)
            {
                const ResourceMaterial* constMat = mesh->GetResourceMaterial();
                if (constMat)
                {
                    ResourceMaterial* teleportWarningScreamMaterial = dynamic_cast<ResourceMaterial*>(
                        AppEngine->GetResourcesModule()->RequestResource(constMat->GetUID())
                    );

                    teleportWarningScreamMaterial->SetDiffColor(screamWarningColor);
                }
            }
        }
        else if (currentGO->GetName() == "TeleportWarningSlow")
        {
            teleportWarningSlowGO = currentGO;
            teleportWarningSlowGO->SetEnabled(false);

            MeshComponent* mesh = teleportWarningSlowGO->GetComponent<MeshComponent*>();
            if (mesh)
            {
                const ResourceMaterial* constMat = mesh->GetResourceMaterial();
                if (constMat)
                {
                    ResourceMaterial* teleportWarningSlowMaterial = dynamic_cast<ResourceMaterial*>(
                        AppEngine->GetResourcesModule()->RequestResource(constMat->GetUID())
                    );

                    teleportWarningSlowMaterial->SetDiffColor(slowWarningColor);
                }
            }
        }
        else if (currentGO->GetName() == "PS_BansheeHit")
        {
            hitParticleSystem = currentGO->GetComponent<ParticleSystemComponent*>();
        }
        else if (currentGO->GetName() == "VFX_Death_Spritesheet")
        {
            deathVFXShaderComponents = currentGO->GetAllComponentsInChilds<ShaderScriptComponent*>(AppEngine);

            for (ShaderScriptComponent* shaderComp : deathVFXShaderComponents)
            {
                shaderComp->SetEnabled(false);
            }
        }
        else if (currentGO->GetName() == "VFX_Hit_Spritesheet")
        {
            hitVFXShaderComponents = currentGO->GetAllComponentsInChilds<ShaderScriptComponent*>(AppEngine);

            for (ShaderScriptComponent* shaderComp : hitVFXShaderComponents)
            {
                shaderComp->SetEnabled(false);
            }
        }
        else if (currentGO->GetName() == "VFX_Teleport_Spritesheet")
        {
            teleportVFXShaderComponents = currentGO->GetAllComponentsInChilds<ShaderScriptComponent*>(AppEngine);

            for (ShaderScriptComponent* shaderComp : teleportVFXShaderComponents)
            {
                shaderComp->SetEnabled(false);
            }
        }
        else if (currentGO->GetName() == "VFX_Forward_Shout")
        {
            forwardScreamShaderComponents = currentGO->GetAllComponentsInChilds<ShaderScriptComponent*>(AppEngine);

            forwardScreamCollider         = currentGO->GetComponent<CapsuleColliderComponent*>();
            forwardScreamCollider->SetEnabled(false);

            for (ShaderScriptComponent* shaderComponent : forwardScreamShaderComponents)
            {
                shaderComponent->SetScriptEnabled("MovingUVTransparent", false);
            }
        }
        else if (currentGO->GetName() == "VFX_Ground_Ring")
        {
            groundRingShaderComponents = currentGO->GetAllComponentsInChilds<ShaderScriptComponent*>(AppEngine);

            for (auto& shaderComponent : groundRingShaderComponents)
            {
                shaderComponent->SetScriptEnabled("MovingUVTransparent", false);
            }
        }
        else if (currentGO->GetName() == "ScreamWarning")
        {
            screamAreaWarningGO = currentGO;

            float3 translation, scale;
            Quat rotation;

            screamAreaWarningGO->GetLocalTransform().Decompose(translation, rotation, scale);
            float4x4 starTransform =
                float4x4::FromTRS(translation, rotation, float3(slowAreaWaringMaxScale, 1.f, slowAreaWaringMaxScale));
            screamAreaWarningGO->SetLocalTransform(starTransform);
            screamAreaWarningGO->SetEnabled(false);
        }
    }

    rng            = std::mt19937(std::random_device {}());
    normalizedDist = std::uniform_real_distribution<float>(0.0f, 1.0f);
    invisibleDist  = std::uniform_real_distribution<float>(invisibleTimeRange[0], invisibleTimeRange[1]);

    return true;
}

void Banshee::Update(float deltaTime)
{
    if (agentAI == nullptr) return;

    Character::Update(deltaTime);

    if (AppEngine->GetDebugDrawModule()->GetDebugOptionValue((int)DebugOptions::RENDER_DEBUG_VISUALS))
    {
        const std::string life         = "Health: " + std::to_string(currentHealth);
        const std::string animState    = "Anim state: " + stateName.GetString();

        std::string currentStateString = BansheeStateStrings[(int)currentState];
        const std::string logicState   = "Logic state: " + currentStateString;

        std::vector<std::pair<std::string, float2>> logs {
            {life,       float2(-50.0f, -140.0f)},
            {animState,  float2(-80.0f, -160.0f)},
            {logicState, float2(-80.0f, -180.0f)},
        };

        RenderDebug(logs, float3(1.0f, 0.0f, 0.0f));
    }
}

void Banshee::OnPlayerExitLocation()
{
    switch (currentState)
    {
    case BansheeStates::Search:
        isSearching = false;
        break;
    case BansheeStates::Attack:
    {
        isInvisible = false;
        mesh->SetEnabled(true);
        characterCollider->SetEnabled(true);

        weapon->SetEnabled(false);

        // FORWARD SCREAM
        forwardScreamCollider->SetEnabled(false);
        for (ShaderScriptComponent* shaderComponent : forwardScreamShaderComponents)
        {
            shaderComponent->SetScriptEnabled("MovingUVTransparent", false);
        }

        // GROUND RING

        for (auto& shaderComponent : groundRingShaderComponents)
        {
            shaderComponent->SetScriptEnabled("MovingUVTransparent", false);
        }

        for (auto& shaderComponent : groundRingShaderComponents)
        {
            shaderComponent->ResetScript("MovingUVTransparent");
        }

        float3 translation, scale;
        Quat rotation;

        screamAreaWarningGO->GetLocalTransform().Decompose(translation, rotation, scale);
        float4x4 starTransform =
            float4x4::FromTRS(translation, rotation, float3(slowAreaWaringMaxScale, 1.f, slowAreaWaringMaxScale));
        screamAreaWarningGO->SetLocalTransform(starTransform);
        screamAreaWarningGO->SetEnabled(false);

        teleportWarningScreamGO->SetEnabled(false);

        isAttacking = false;
        agentAI->ResetSpeed();
        agentAI->ResetAngularSpeed();
        agentAI->SetLookForward(true);

        break;
    }
    case BansheeStates::SlowArea:
    {
        isInvisible = false;
        mesh->SetEnabled(true);
        characterCollider->SetEnabled(true);

        slowAreaGO->SetEnabled(false);
        slowAreaInGO->SetEnabled(false);

        float3 translation, scale;
        Quat rotation;

        slowAreaWarningGO->GetLocalTransform().Decompose(translation, rotation, scale);
        float4x4 starTransform =
            float4x4::FromTRS(translation, rotation, float3(slowAreaWaringMaxScale, 1.f, slowAreaWaringMaxScale));
        slowAreaWarningGO->SetLocalTransform(starTransform);
        slowAreaWarningGO->SetEnabled(false);

        teleportWarningSlowGO->SetEnabled(false);

        isAttacking = false;
        agentAI->ResetSpeed();
        agentAI->ResetAngularSpeed();
        agentAI->SetLookForward(true);

        break;
    }
    default:
        break;
    }
    currentState = BansheeStates::TeleportOrigin;
}

void Banshee::OnDeath()
{
    parent->SetEnabled(false);
}

void Banshee::OnDamageTaken(int amount)
{
    // if (hitParticleSystem) hitParticleSystem->SpawnAllInstances();

    for (ShaderScriptComponent* shaderComp : hitVFXShaderComponents)
    {
        shaderComp->SetEnabled(true);
        shaderComp->ResetScript("AttackVfxSpritesheet");
    }
}

void Banshee::PerformAttack()
{
}

void Banshee::HandleState(float deltaTime)
{
    switch (currentState)
    {
    case BansheeStates::Idle:
        if (animComponent) animComponent->UseTrigger("Idle");
        ChangeState();
        break;

    case BansheeStates::Search:
        SearchForPlayer();
        break;

    case BansheeStates::Chase:
        ChasePlayer();
        break;

    case BansheeStates::Attack:
        Attack(deltaTime);
        break;

    case BansheeStates::Hit:
        if (animComponent->GetCurrentStateName() == HashString("Hit") && animComponent->IsFinished())
        {
            animComponent->UseTrigger("Idle");
            currentState = BansheeStates::Idle;

            agentAI->ResumeMovement();
            ChangeState();
        }
        break;

    case BansheeStates::Dead:
        HandleDeath();
        break;

    case BansheeStates::TeleportOrigin:
        TeleportToOrigin();
        break;

    case BansheeStates::SlowArea:
        SlowArea(deltaTime);
        break;

    default:
        currentState = BansheeStates::Idle;
        ChangeState();
        break;
    }
}

void Banshee::TakeDamage(int amount)
{
    if (isInvisible) return;

    switch (currentState)
    {
    case BansheeStates::Attack:

    {
        animComponent->UseTrigger("Hit");
        currentState = BansheeStates::Hit;

        weapon->SetEnabled(false);

        // FORWARD SCREAM
        forwardScreamCollider->SetEnabled(false);
        for (ShaderScriptComponent* shaderComponent : forwardScreamShaderComponents)
        {
            shaderComponent->SetScriptEnabled("MovingUVTransparent", false);
        }

        // GROUND RING

        for (auto& shaderComponent : groundRingShaderComponents)
        {
            shaderComponent->SetScriptEnabled("MovingUVTransparent", false);
        }

        for (auto& shaderComponent : groundRingShaderComponents)
        {
            shaderComponent->ResetScript("MovingUVTransparent");
        }

        float3 translation, scale;
        Quat rotation;

        screamAreaWarningGO->GetLocalTransform().Decompose(translation, rotation, scale);
        float4x4 starTransform =
            float4x4::FromTRS(translation, rotation, float3(slowAreaWaringMaxScale, 1.f, slowAreaWaringMaxScale));
        screamAreaWarningGO->SetLocalTransform(starTransform);
        screamAreaWarningGO->SetEnabled(false);

        slowAreaGO->SetEnabled(false);

        isAttacking = false;
        agentAI->ResetSpeed();
        agentAI->ResetAngularSpeed();
        agentAI->SetLookForward(true);
    }

    break;
    case BansheeStates::Hit:
        if (animComponent->GetCurrentStateName() == HashString("Hit") && animComponent->IsFinished())
        {
            animComponent->UseTrigger("Idle");
            currentState = BansheeStates::Idle;

            agentAI->ResumeMovement();
        }
        break;
    case BansheeStates::SlowArea:
    {
        animComponent->UseTrigger("Hit");
        currentState = BansheeStates::Hit;

        slowAreaGO->SetEnabled(false);
        slowAreaInGO->SetEnabled(false);

        float3 translation, scale;
        Quat rotation;

        slowAreaWarningGO->GetLocalTransform().Decompose(translation, rotation, scale);
        float4x4 starTransform =
            float4x4::FromTRS(translation, rotation, float3(slowAreaWaringMaxScale, 1.f, slowAreaWaringMaxScale));
        slowAreaWarningGO->SetLocalTransform(starTransform);
        slowAreaWarningGO->SetEnabled(false);

        isAttacking = false;
        agentAI->ResetSpeed();
        agentAI->ResetAngularSpeed();
        agentAI->SetLookForward(true);

        break;
    }
    case BansheeStates::Dead:
        break;
    default:
    {
        isSearching = false;
        animComponent->UseTrigger("Hit");
        currentState = BansheeStates::Hit;
        agentAI->PauseMovement();
        break;
    }
    }

    if ((currentHealth - amount) <= 0)
    {

        for (ShaderScriptComponent* shaderComp : deathVFXShaderComponents)
        {
            shaderComp->SetEnabled(true);
        }

        animComponent->UseTrigger("Death");
        currentState = BansheeStates::Dead;
        return;
    }

    Character::TakeDamage(amount);
}

void Banshee::ChasePlayer()
{
    if (!character) return;

    hasMoved = true;
    if (animComponent) animComponent->UseTrigger("Chase");
    if (CheckDistanceWithPlayer() <= PlayerDistances::Close)
    {
        float attackToPerform = normalizedDist(rng);

        if (attackToPerform < 0.5f) currentState = BansheeStates::Attack;
        else currentState = BansheeStates::SlowArea;
    }
    else if (!agentAI->SetPathNavigation(character->GetLastPosition()) || GetDistanceFromPlayer() > maxDetectionRange)
        currentState = BansheeStates::Search;
}

void Banshee::Attack(float deltaTime)
{
    if (!weapon) return;

    if (!isAttacking)
    {
        hasMoved = true;
        // GLOG("Banshee attack");
        agentAI->SetLookForward(false);

        Character::Attack(deltaTime);
        agentAI->SetSpeed(0.0f, 0.0f);

        isInvisible = false;
        animComponent->UseTrigger("Teleport");
    }
    else
    {
        if (animComponent->GetCurrentStateName() == HashString("Teleport") && animComponent->IsFinished() &&
            !isInvisible)
        {
            // Teleport VFX
            for (ShaderScriptComponent* shaderComp : teleportVFXShaderComponents)
            {
                shaderComp->SetEnabled(true);
                shaderComp->ResetScript("AttackVfxSpritesheet");
            }

            currentInvisibleTime = invisibleDist(rng) + teleportVFXDuration;
            isInvisible          = true;
            mesh->SetEnabled(false);
            characterCollider->SetEnabled(false);

            elapsedTeleportVFX = 0.0f;

            teleportedToPos    = false;
        }
        if (elapsedTeleportVFX < teleportVFXDuration)
        {
            elapsedTeleportVFX += deltaTime;
            return;
        }
        else if (elapsedTeleportVFX > teleportVFXDuration && !teleportedToPos)
        {
            teleportWarningScreamGO->SetEnabled(true);
            GoToAttackPosition();
            teleportedToPos = true;
            return;
        }
        else if (attackTimer < currentInvisibleTime)
        {
            // SCALING WARNING OVER TIME
            float3 translation, scale;
            Quat rotation;

            teleportWarningScreamGO->GetLocalTransform().Decompose(translation, rotation, scale);

            float interpolationValue = min(attackTimer / currentInvisibleTime, 1.f);

            float finalScale         = ImGui::CurveValue(interpolationValue, maxScriptCurvePoints, curveEditorPoints);

            scale                    = float3(finalScale, 1.f, finalScale);

            float4x4 starTransform   = float4x4::FromTRS(translation, rotation, scale);
            teleportWarningScreamGO->SetLocalTransform(starTransform);

            return;
        }

        if (isInvisible)
        {
            teleportWarningScreamGO->SetEnabled(false);

            // Tp to player and enable
            // GoToAttackPosition();

            mesh->SetEnabled(true);
            characterCollider->SetEnabled(true);
            isInvisible = false;
            agentAI->SetAngularSpeed(attackAngularSpeed);
            animComponent->UseTrigger("ScreamIn");

            slowAreaGO->SetEnabled(true);
            slowAreaGO->SetLocalPosition(float3::zero);

            elapsedWarning = 0.f;
            screamAreaWarningGO->SetEnabled(true);
        }

        // Slowly rotate towards player while charging the attack
        else if (animComponent->GetCurrentStateName() == HashString("ScreamIn") && !animComponent->IsFinished())
        {
            elapsedWarning += deltaTime;

            // SCALING WARNING OVER TIME
            float3 translation, scale;
            Quat rotation;

            screamAreaWarningGO->GetLocalTransform().Decompose(translation, rotation, scale);

            float interpolationValue = min(elapsedWarning / warningDuration, 1.f);

            float finalScale         = Interpolation::Lerp(slowAreaWaringMaxScale, 0.1f, interpolationValue);

            scale                    = float3(finalScale, 1.f, finalScale);

            float4x4 starTransform   = float4x4::FromTRS(translation, rotation, scale);
            screamAreaWarningGO->SetLocalTransform(starTransform);

            agentAI->LookAtMovement(character->GetLastPosition(), deltaTime);
        }

        else if (animComponent->GetCurrentStateName() == HashString("ScreamIn") && animComponent->IsFinished())
        {
            // Reseting scale.
            float3 translation, scale;
            Quat rotation;

            screamAreaWarningGO->GetLocalTransform().Decompose(translation, rotation, scale);
            float4x4 starTransform =
                float4x4::FromTRS(translation, rotation, float3(slowAreaWaringMaxScale, 1.f, slowAreaWaringMaxScale));
            screamAreaWarningGO->SetLocalTransform(starTransform);
            screamAreaWarningGO->SetEnabled(false);

            animComponent->UseTrigger("Scream");

            weapon->SetEnabled(true);

            // FORWARD SCREAM ENABLE

            forwardScreamCollider->SetEnabled(true);
            for (ShaderScriptComponent* shaderComponent : forwardScreamShaderComponents)
            {
                shaderComponent->SetScriptEnabled("MovingUVTransparent", true);
            }

            // GROUND RING ENABLE

            for (auto& shaderComponent : groundRingShaderComponents)
            {
                shaderComponent->SetScriptEnabled("MovingUVTransparent", true);
            }

            elapsedMainScream = 0.f;
        }

        else if (animComponent->GetCurrentStateName() == HashString("Scream") && elapsedMainScream < mainScreamDuration)
            elapsedMainScream += deltaTime;

        else if (animComponent->GetCurrentStateName() == HashString("Scream") &&
                 elapsedMainScream >= mainScreamDuration)
        {

            animComponent->UseTrigger("ScreamOut");

            weapon->SetEnabled(false);

            elapsedMainScream = 0.f;

            // FORWARD SCREAM DISABLE

            forwardScreamCollider->SetEnabled(false);
            for (ShaderScriptComponent* shaderComponent : forwardScreamShaderComponents)
            {
                shaderComponent->SetScriptEnabled("MovingUVTransparent", false);
            }

            // GROUND RING DISABLE

            for (auto& shaderComponent : groundRingShaderComponents)
            {
                shaderComponent->SetScriptEnabled("MovingUVTransparent", false);
            }

            for (auto& shaderComponent : groundRingShaderComponents)
            {
                shaderComponent->ResetScript("MovingUVTransparent");
            }

            slowAreaGO->SetEnabled(false);
        }
        else if (animComponent->GetCurrentStateName() == HashString("ScreamOut") && animComponent->IsFinished())
        {
            isAttacking  = false;
            currentState = BansheeStates::Idle;
            animComponent->UseTrigger("Idle");

            agentAI->ResetSpeed();
            agentAI->ResetAngularSpeed();
            agentAI->SetLookForward(true);
        }
    }
}

void Banshee::ChangeState()
{
    if (playerScript->IsDead())
    {
        currentState = BansheeStates::Idle;
        return;
    }

    const float distance = GetDistanceFromPlayer();
    if (distance <= rangeAIAttack)
    {
        float attackToPerform = normalizedDist(rng);

        if (attackToPerform < 0.5f) currentState = BansheeStates::Attack;
        else currentState = BansheeStates::SlowArea;
    }
    else if (distance <= rangeAIChase) currentState = BansheeStates::Chase;
    else currentState = BansheeStates::Search;
}

void Banshee::SearchForPlayer()
{
    if (!isSearching)
    {
        firstSearch = true;
        isSearching = true;
        animComponent->UseTrigger("SearchRight");
        agentAI->SetSpeed(0.0f, 0.0f);
    }
    else
    {
        if (GetDistanceFromPlayer() < maxDetectionRange - 0.5f)
        {
            isSearching = false;
            agentAI->ResetSpeed();
            currentState = BansheeStates::Chase;
            return;
        }

        if (firstSearch && animComponent->IsFinished())
        {
            firstSearch = false;
            animComponent->UseTrigger("SearchLeft");
        }
        else if (animComponent->IsFinished())
        {
            isSearching = false;
            if (hasMoved) currentState = BansheeStates::TeleportOrigin;
            else currentState = BansheeStates::Idle;
        }
    }
}

void Banshee::GoToAttackPosition()
{
    const float3 playerPos = character->GetLastPosition();
    const float maxRadius  = 2.5f;
    const float minRadius  = 1.5f;

    // Get a random position within a circle smaller than maxRadius and bigger than minRadius
    const float angle      = normalizedDist(rng) * 2.0f * PI;
    const float r =
        sqrtf(normalizedDist(rng) * (maxRadius * maxRadius - minRadius * minRadius) + minRadius * minRadius);

    const float3 position(cosf(angle) * r + playerPos.x, playerPos.y, sinf(angle) * r + playerPos.z);

    agentAI->SetPosition(position);
    agentAI->LookAtMovement(character->GetLastPosition(), 1.0f);
}

void Banshee::TeleportToOrigin()
{
    if (animComponent->GetCurrentStateName() != HashString("Teleport"))
    {
        animComponent->UseTrigger("Teleport");
    }
    else if (animComponent->GetCurrentStateName() == HashString("Teleport") && animComponent->IsFinished())
    {
        agentAI->SetPosition(startPos);
        currentState = BansheeStates::Idle;
        animComponent->UseTrigger("Idle");
        hasMoved = false;
    }
}

void Banshee::HandleDeath()
{
    if (animComponent->GetCurrentStateName() == HashString("Death") && animComponent->IsFinished())
    {
        currentHealth = 0;
        Character::Die();
    }
}

void Banshee::SlowArea(float deltaTime)
{
    if (!slowAreaGO) return;

    if (!isAttacking)
    {
        hasMoved = true;
        agentAI->SetLookForward(false);

        Character::Attack(deltaTime);
        agentAI->SetSpeed(0.0f, 0.0f);

        isInvisible = false;
        animComponent->UseTrigger("Teleport");
    }
    else
    {
        if (animComponent->GetCurrentStateName() == HashString("Teleport") && animComponent->IsFinished() &&
            !isInvisible)
        {
            // Teleport VFX
            for (ShaderScriptComponent* shaderComp : teleportVFXShaderComponents)
            {
                shaderComp->SetEnabled(true);
                shaderComp->ResetScript("AttackVfxSpritesheet");
            }

            currentInvisibleTime = invisibleDist(rng) + teleportVFXDuration;
            isInvisible          = true;

            mesh->SetEnabled(false);
            characterCollider->SetEnabled(false);

            elapsedTeleportVFX = 0.0f;
            teleportedToPos    = false;
        }
        if (elapsedTeleportVFX < teleportVFXDuration)
        {
            elapsedTeleportVFX += deltaTime;
            return;
        }
        else if (elapsedTeleportVFX > teleportVFXDuration && !teleportedToPos)
        {
            GoToAttackPosition();
            teleportWarningSlowGO->SetEnabled(true);
            teleportedToPos = true;
            return;
        }
        else if (attackTimer < currentInvisibleTime)
        {
            // SCALING WARNING OVER TIME
            float3 translation, scale;
            Quat rotation;

            teleportWarningSlowGO->GetLocalTransform().Decompose(translation, rotation, scale);

            float interpolationValue = min(attackTimer / currentInvisibleTime, 1.f);

            float finalScale         = ImGui::CurveValue(interpolationValue, maxScriptCurvePoints, curveEditorPoints);

            scale                    = float3(finalScale, 1.f, finalScale);

            float4x4 starTransform   = float4x4::FromTRS(translation, rotation, scale);
            teleportWarningSlowGO->SetLocalTransform(starTransform);

            return;
        }

        if (isInvisible)
        {
            teleportWarningSlowGO->SetEnabled(false);

            mesh->SetEnabled(true);
            characterCollider->SetEnabled(true);
            isInvisible = false;
            agentAI->SetAngularSpeed(attackAngularSpeed);
            animComponent->UseTrigger("ScreamIn");

            UpdateLastPlayerPosition();
            MoveSlowAreaToPlayer();

            elapsedSlowAreaWaring = 0.f;
            slowAreaWarningGO->SetEnabled(true);
            slowAreaGO->SetEnabled(true);
        }
        else if (animComponent->GetCurrentStateName() == HashString("ScreamIn") && !animComponent->IsFinished())
        {
            elapsedSlowAreaWaring += deltaTime;

            // SCALING WARNING OVER TIME
            float3 translation, scale;
            Quat rotation;

            slowAreaWarningGO->GetLocalTransform().Decompose(translation, rotation, scale);

            float interpolationValue = min(elapsedSlowAreaWaring / slowAreaWaringDuration, 1.f);

            float finalScale         = Interpolation::Lerp(slowAreaWaringMaxScale, 0.1f, interpolationValue);

            scale                    = float3(finalScale, 1.f, finalScale);

            float4x4 starTransform   = float4x4::FromTRS(translation, rotation, scale);
            slowAreaWarningGO->SetLocalTransform(starTransform);

            agentAI->LookAtMovement(character->GetLastPosition(), deltaTime);

            MoveSlowAreaToPlayer();
        }

        else if (animComponent->GetCurrentStateName() == HashString("ScreamIn") && animComponent->IsFinished())
        {
            // Reseting scale.
            float3 translation, scale;
            Quat rotation;

            slowAreaWarningGO->GetLocalTransform().Decompose(translation, rotation, scale);
            float4x4 starTransform =
                float4x4::FromTRS(translation, rotation, float3(slowAreaWaringMaxScale, 1.f, slowAreaWaringMaxScale));
            slowAreaWarningGO->SetLocalTransform(starTransform);
            slowAreaWarningGO->SetEnabled(false);

            animComponent->UseTrigger("SlowArea");
            slowAreaInGO->SetEnabled(true);

            elapsedSlowArea = 0.f;
        }
        else if (animComponent->GetCurrentStateName() == HashString("SlowArea") && elapsedSlowArea < slowAreaDuration)
            elapsedSlowArea += deltaTime;

        else if (animComponent->GetCurrentStateName() == HashString("SlowArea") && elapsedSlowArea >= slowAreaDuration)
        {
            slowAreaGO->SetEnabled(false);
            slowAreaInGO->SetEnabled(false);
            animComponent->UseTrigger("ScreamOut");
        }
        else if (animComponent->GetCurrentStateName() == HashString("ScreamOut") && animComponent->IsFinished())
        {
            isAttacking  = false;
            currentState = BansheeStates::Idle;
            animComponent->UseTrigger("Idle");

            agentAI->ResetSpeed();
            agentAI->ResetAngularSpeed();
            agentAI->SetLookForward(true);
        }
    }
}

void Banshee::MoveSlowAreaToPlayer()
{
    if (!character) return;

    float3 translate, scale;
    Quat rotation;

    float4x4 parentInvertedGlobal = parent->GetGlobalTransform().Inverted();

    slowAreaGO->GetGlobalTransform().Decompose(translate, rotation, scale);
    float4x4 newGlobalTransform = float4x4::FromTRS(
        float3(lastPlayerPosition.x, lastPlayerPosition.y + slowAreaStartHeight, lastPlayerPosition.z), rotation, scale
    );
    slowAreaGO->SetLocalTransform(parentInvertedGlobal * newGlobalTransform);

    slowAreaInGO->GetGlobalTransform().Decompose(translate, rotation, scale);
    newGlobalTransform = float4x4::FromTRS(
        float3(lastPlayerPosition.x, lastPlayerPosition.y + slowAreaInStartHeight, lastPlayerPosition.z), rotation,
        scale
    );
    slowAreaInGO->SetLocalTransform(parentInvertedGlobal * newGlobalTransform);

    slowAreaWarningGO->GetGlobalTransform().Decompose(translate, rotation, scale);
    newGlobalTransform = float4x4::FromTRS(
        float3(lastPlayerPosition.x, lastPlayerPosition.y + slowWarningStartHeight, lastPlayerPosition.z), rotation,
        scale
    );
    slowAreaWarningGO->SetLocalTransform(parentInvertedGlobal * newGlobalTransform);
}

void Banshee::UpdateLastPlayerPosition()
{
    if (!character)
    {
        lastPlayerPosition = parent->GetGlobalTransform().TranslatePart();
        return;
    }

    lastPlayerPosition = character->GetLastPosition();
}