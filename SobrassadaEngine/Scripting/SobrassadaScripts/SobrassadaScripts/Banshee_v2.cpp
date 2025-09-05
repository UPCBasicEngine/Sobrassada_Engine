#include "pch.h"

#include "Banshee_v2.h"

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
#include "Standalone/Physics/SphereColliderComponent.h"

#include "imgui_curve_editor.h"

Banshee_v2::Banshee_v2(GameObject* parent)
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

bool Banshee_v2::Init()
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
        else if (currentGO->GetName() == "VFX_Banshee_shoutStart")
        {
            shoutStartAnim             = currentGO->GetComponent<AnimationComponent*>();

            shoutStartShaderComponents = currentGO->GetAllComponentsInChilds<ShaderScriptComponent*>(AppEngine);
            auto meshComponents        = currentGO->GetAllComponentsInChilds<MeshComponent*>(AppEngine);

            for (ShaderScriptComponent* shaderComponent : shoutStartShaderComponents)
            {
                shaderComponent->SetScriptEnabled("MovingUVTransparent", false);
            }

            for (MeshComponent* meshComp : meshComponents)
            {
                if (meshComp->GetParent()->GetName() == "mesh_stars")
                {
                    shoutStartMeshComponents.push_back(meshComp);
                }
            }

            shoutStartAnim->GetParent()->SetEnabled(false);
        }
        else if (currentGO->GetName() == "VFX_Banshee_shoutBase")
        {
            shoutBaseAnim             = currentGO->GetComponent<AnimationComponent*>();

            shoutBaseShaderComponents = currentGO->GetAllComponentsInChilds<ShaderScriptComponent*>(AppEngine);
            auto meshComponents       = currentGO->GetAllComponentsInChilds<MeshComponent*>(AppEngine);

            for (ShaderScriptComponent* shaderComponent : shoutBaseShaderComponents)
            {
                shaderComponent->SetScriptEnabled("MovingUVTransparent", false);
            }

            for (MeshComponent* meshComp : meshComponents)
            {
                if (meshComp->GetParent()->GetName() == "mesh_general_glow_1")
                    shoutBaseMeshComponents.push_back(meshComp);
                else if (meshComp->GetParent()->GetName() == "mesh_general_glow_2")
                    shoutBaseMeshComponents.push_back(meshComp);
                else if (meshComp->GetParent()->GetName() == "mesh_dark_glow")
                    shoutBaseMeshComponents.push_back(meshComp);
            }

            shoutBaseAnim->GetParent()->SetEnabled(false);
        }
        else if (currentGO->GetName() == "SlowArea")
        {
            slowAreaGO = currentGO;
            slowAreaGO->SetEnabled(false);
        }
        else if (currentGO->GetName() == "SlowAreaIn")
        {
            slowAreaInGO = currentGO;
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
    }

    rng            = std::mt19937(std::random_device {}());
    normalizedDist = std::uniform_real_distribution<float>(0.0f, 1.0f);
    invisibleDist  = std::uniform_real_distribution<float>(invisibleTimeRange[0], invisibleTimeRange[1]);

    return true;
}

void Banshee_v2::Update(float deltaTime)
{
    if (agentAI == nullptr) return;

    Character::Update(deltaTime);

    if (AppEngine->GetDebugDrawModule()->GetDebugOptionValue((int)DebugOptions::RENDER_DEBUG_VISUALS))
    {
        const std::string life         = "Health: " + std::to_string(currentHealth);
        const std::string animState    = "Anim state: " + stateName.GetString();

        std::string currentStateString = Banshee_v2_StateStrings[(int)currentState];
        const std::string logicState   = "Logic state: " + currentStateString;

        std::vector<std::pair<std::string, float2>> logs {
            {life,       float2(-50.0f, -140.0f)},
            {animState,  float2(-80.0f, -160.0f)},
            {logicState, float2(-80.0f, -180.0f)},
        };

        RenderDebug(logs, float3(1.0f, 0.0f, 0.0f));
    }
}

void Banshee_v2::OnPlayerExitLocation()
{
    switch (currentState)
    {
    case Banshee_v2_States::Search:
        isSearching = false;
        break;
    case Banshee_v2_States::Attack:
    {
        isInvisible = false;
        mesh->SetEnabled(true);
        characterCollider->SetEnabled(true);

        for (ShaderScriptComponent* shaderComponent : shoutStartShaderComponents)
        {
            shaderComponent->SetScriptEnabled("MovingUVTransparent", false);
        }

        for (ShaderScriptComponent* shaderComponent : shoutStartShaderComponents)
        {
            shaderComponent->ResetScript("MovingUVTransparent");
        }

        for (ShaderScriptComponent* shaderComponent : shoutBaseShaderComponents)
        {
            shaderComponent->SetScriptEnabled("MovingUVTransparent", false);
        }

        for (ShaderScriptComponent* shaderComponent : shoutBaseShaderComponents)
        {
            shaderComponent->ResetScript("MovingUVTransparent");
        }

        weapon->SetEnabled(false);

        shoutStartAnim->UseTrigger("Reset");
        shoutStartAnim->GetParent()->SetEnabled(false);

        shoutBaseAnim->GetParent()->SetEnabled(false);

        teleportWarningScreamGO->SetEnabled(false);

        isAttacking = false;
        agentAI->ResetSpeed();
        agentAI->ResetAngularSpeed();
        agentAI->SetLookForward(true);

        break;
    }
    case Banshee_v2_States::SlowArea:
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
    currentState = Banshee_v2_States::TeleportOrigin;
}

void Banshee_v2::OnDeath()
{
    parent->SetEnabled(false);
}

void Banshee_v2::OnDamageTaken(int amount)
{
    if (hitParticleSystem) hitParticleSystem->SpawnAllInstances();
}

void Banshee_v2::PerformAttack()
{
}

void Banshee_v2::HandleState(float deltaTime)
{
    switch (currentState)
    {
    case Banshee_v2_States::Idle:
        if (animComponent) animComponent->UseTrigger("Idle");
        ChangeState();
        break;

    case Banshee_v2_States::Search:
        SearchForPlayer();
        break;

    case Banshee_v2_States::Chase:
        ChasePlayer();
        break;

    case Banshee_v2_States::Attack:
        Attack(deltaTime);
        break;

    case Banshee_v2_States::Hit:
        if (animComponent->GetCurrentStateName() == HashString("Hit") && animComponent->IsFinished())
        {
            animComponent->UseTrigger("Idle");
            currentState = Banshee_v2_States::Idle;

            agentAI->ResumeMovement();
            ChangeState();
        }
        break;

    case Banshee_v2_States::Dead:
        HandleDeath();
        break;

    case Banshee_v2_States::TeleportOrigin:
        TeleportToOrigin();
        break;

    case Banshee_v2_States::SlowArea:
        SlowArea(deltaTime);
        break;

    default:
        currentState = Banshee_v2_States::Idle;
        ChangeState();
        break;
    }
}

void Banshee_v2::TakeDamage(int amount)
{
    if (isInvisible) return;

    switch (currentState)
    {
    case Banshee_v2_States::Attack:

    {
        animComponent->UseTrigger("Hit");
        currentState = Banshee_v2_States::Hit;

        shoutStartAnim->UseTrigger("Reset");

        for (ShaderScriptComponent* shaderComponent : shoutStartShaderComponents)
        {
            shaderComponent->SetScriptEnabled("MovingUVTransparent", false);
        }

        for (ShaderScriptComponent* shaderComponent : shoutStartShaderComponents)
        {
            shaderComponent->ResetScript("MovingUVTransparent");
        }

        shoutStartAnim->GetParent()->SetEnabled(false);

        for (ShaderScriptComponent* shaderComponent : shoutBaseShaderComponents)
        {
            shaderComponent->SetScriptEnabled("MovingUVTransparent", false);
        }

        for (ShaderScriptComponent* shaderComponent : shoutBaseShaderComponents)
        {
            shaderComponent->ResetScript("MovingUVTransparent");
        }

        weapon->SetEnabled(false);

        shoutBaseAnim->GetParent()->SetEnabled(false);

        slowAreaGO->SetEnabled(false);

        isAttacking = false;
        agentAI->ResetSpeed();
        agentAI->ResetAngularSpeed();
        agentAI->SetLookForward(true);
    }

    break;
    case Banshee_v2_States::Hit:
        if (animComponent->GetCurrentStateName() == HashString("Hit") && animComponent->IsFinished())
        {
            animComponent->UseTrigger("Idle");
            currentState = Banshee_v2_States::Idle;

            agentAI->ResumeMovement();
        }
        break;
    case Banshee_v2_States::SlowArea:
    {
        animComponent->UseTrigger("Hit");
        currentState = Banshee_v2_States::Hit;

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
    case Banshee_v2_States::Dead:
        break;
    default:
    {
        isSearching = false;
        animComponent->UseTrigger("Hit");
        currentState = Banshee_v2_States::Hit;
        agentAI->PauseMovement();
        break;
    }
    }

    if ((currentHealth - amount) <= 0)
    {
        animComponent->UseTrigger("Death");
        currentState = Banshee_v2_States::Dead;
        return;
    }

    Character::TakeDamage(amount);
}

void Banshee_v2::ChasePlayer()
{
    if (!character) return;

    hasMoved = true;
    if (animComponent) animComponent->UseTrigger("Chase");
    if (CheckDistanceWithPlayer() <= PlayerDistances::Close)
    {
        float attackToPerform = normalizedDist(rng);

        if (attackToPerform < 0.5f) currentState = Banshee_v2_States::Attack;
        else currentState = Banshee_v2_States::SlowArea;
    }
    else if (!agentAI->SetPathNavigation(character->GetLastPosition()) || GetDistanceFromPlayer() > maxDetectionRange)
        currentState = Banshee_v2_States::Search;
}

void Banshee_v2::Attack(float deltaTime)
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
            currentInvisibleTime = invisibleDist(rng);
            isInvisible          = true;
            mesh->SetEnabled(false);
            characterCollider->SetEnabled(false);

            GoToAttackPosition();

            teleportWarningScreamGO->SetEnabled(true);
        }
        if (attackTimer < currentInvisibleTime)
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

            shoutStartAnim->GetParent()->SetEnabled(true);

            for (MeshComponent* meshComp : shoutStartMeshComponents)
            {
                meshComp->SetEnabled(false);
            }

            shoutStartAnim->UseTrigger("Reset");
            shoutStartAnim->OnPlay(false);

            slowAreaGO->SetEnabled(true);
            slowAreaGO->SetLocalPosition(float3::zero);

            elapsedWarning = 0.f;
        }

        // Slowly rotate towards player while charging the attack
        else if (animComponent->GetCurrentStateName() == HashString("ScreamIn") && !animComponent->IsFinished())
        {
            agentAI->LookAtMovement(character->GetLastPosition(), deltaTime);

            if (elapsedWarning < warningDuration) elapsedWarning += deltaTime;
            else
            {
                for (ShaderScriptComponent* shaderComponent : shoutStartShaderComponents)
                {
                    shaderComponent->SetScriptEnabled("MovingUVTransparent", true);
                }
            }
        }

        else if (animComponent->GetCurrentStateName() == HashString("ScreamIn") && animComponent->IsFinished())
        {
            animComponent->UseTrigger("Scream");

            weapon->SetEnabled(true);
            // SHOUT START DISABLE
            for (ShaderScriptComponent* shaderComponent : shoutStartShaderComponents)
            {
                shaderComponent->SetScriptEnabled("MovingUVTransparent", false);
            }

            for (ShaderScriptComponent* shaderComponent : shoutStartShaderComponents)
            {
                shaderComponent->ResetScript("MovingUVTransparent");
            }

            shoutStartAnim->GetParent()->SetEnabled(false);

            // SHOUT BASE ENABLE
            shoutBaseAnim->GetParent()->SetEnabled(true);

            shoutBaseAnim->OnPlay(false);

            for (MeshComponent* meshComp : shoutBaseMeshComponents)
            {
                meshComp->SetEnabled(false);
            }

            for (ShaderScriptComponent* shaderComponent : shoutBaseShaderComponents)
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

            shoutBaseAnim->OnStop();

            for (ShaderScriptComponent* shaderComponent : shoutBaseShaderComponents)
            {
                shaderComponent->SetScriptEnabled("MovingUVTransparent", false);
            }

            for (ShaderScriptComponent* shaderComponent : shoutBaseShaderComponents)
            {
                shaderComponent->ResetScript("MovingUVTransparent");
            }

            shoutBaseAnim->GetParent()->SetEnabled(false);

            slowAreaGO->SetEnabled(false);
        }
        else if (animComponent->GetCurrentStateName() == HashString("ScreamOut") && animComponent->IsFinished())
        {
            isAttacking  = false;
            currentState = Banshee_v2_States::Idle;
            animComponent->UseTrigger("Idle");

            agentAI->ResetSpeed();
            agentAI->ResetAngularSpeed();
            agentAI->SetLookForward(true);
        }
    }
}

void Banshee_v2::ChangeState()
{
    if (playerScript->IsDead())
    {
        currentState = Banshee_v2_States::Idle;
        return;
    }

    const float distance = GetDistanceFromPlayer();
    if (distance <= rangeAIAttack)
    {
        float attackToPerform = normalizedDist(rng);

        if (attackToPerform < 0.5f) currentState = Banshee_v2_States::Attack;
        else currentState = Banshee_v2_States::SlowArea;
    }
    else if (distance <= rangeAIChase) currentState = Banshee_v2_States::Chase;
    else currentState = Banshee_v2_States::Search;
}

void Banshee_v2::SearchForPlayer()
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
            currentState = Banshee_v2_States::Chase;
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
            if (hasMoved) currentState = Banshee_v2_States::TeleportOrigin;
            else currentState = Banshee_v2_States::Idle;
        }
    }
}

void Banshee_v2::GoToAttackPosition()
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

void Banshee_v2::TeleportToOrigin()
{
    if (animComponent->GetCurrentStateName() != HashString("Teleport"))
    {
        animComponent->UseTrigger("Teleport");
    }
    else if (animComponent->GetCurrentStateName() == HashString("Teleport") && animComponent->IsFinished())
    {
        agentAI->SetPosition(startPos);
        currentState = Banshee_v2_States::Idle;
        animComponent->UseTrigger("Idle");
        hasMoved = false;
    }
}

void Banshee_v2::HandleDeath()
{
    if (animComponent->GetCurrentStateName() == HashString("Death") && animComponent->IsFinished())
    {
        currentHealth = 0;
        Character::Die();
    }
}

void Banshee_v2::SlowArea(float deltaTime)
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
            currentInvisibleTime = invisibleDist(rng);
            isInvisible          = true;

            mesh->SetEnabled(false);
            characterCollider->SetEnabled(false);
            GoToAttackPosition();

            teleportWarningSlowGO->SetEnabled(true);
        }
        if (attackTimer < currentInvisibleTime)
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

            elapsedSlowAreaWaring = 0.f;
            slowAreaWarningGO->SetEnabled(true);
            slowAreaGO->SetEnabled(true);

            UpdateLastPlayerPosition();
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
            currentState = Banshee_v2_States::Idle;
            animComponent->UseTrigger("Idle");

            agentAI->ResetSpeed();
            agentAI->ResetAngularSpeed();
            agentAI->SetLookForward(true);
        }
    }
}

void Banshee_v2::MoveSlowAreaToPlayer()
{
    if (!character) return;

    float3 translate, scale;
    Quat rotation;

    float4x4 parentInvertedGlobal = parent->GetGlobalTransform().Inverted();

    slowAreaGO->GetGlobalTransform().Decompose(translate, rotation, scale);
    float4x4 newGlobalTransform = float4x4::FromTRS(lastPlayerPosition, rotation, scale);
    slowAreaGO->SetLocalTransform(parentInvertedGlobal * newGlobalTransform);

    slowAreaInGO->GetGlobalTransform().Decompose(translate, rotation, scale);
    newGlobalTransform = float4x4::FromTRS(lastPlayerPosition, rotation, scale);
    slowAreaInGO->SetLocalTransform(parentInvertedGlobal * newGlobalTransform);

    slowAreaWarningGO->GetGlobalTransform().Decompose(translate, rotation, scale);
    newGlobalTransform = float4x4::FromTRS(lastPlayerPosition, rotation, scale);
    slowAreaWarningGO->SetLocalTransform(parentInvertedGlobal * newGlobalTransform);
}

void Banshee_v2::UpdateLastPlayerPosition()
{
    if (!character)
    {
        lastPlayerPosition = parent->GetGlobalTransform().TranslatePart();
        return;
    }

    lastPlayerPosition = character->GetLastPosition();
}
