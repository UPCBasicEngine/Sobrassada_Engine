#include "pch.h"

#include "Banshee.h"

#include "CuChulainn.h"
#include "DebugDrawModule.h"
#include "GameObject.h"
#include "GameTimer.h"
#include "Interpolation.h"
#include "ShaderScriptComponent.h"
#include "Standalone/AIAgentComponent.h"
#include "Standalone/AnimationComponent.h"
#include "Standalone/CharacterControllerComponent.h"
#include "Standalone/MeshComponent.h"
#include "Standalone/Physics/CapsuleColliderComponent.h"
#include "Standalone/Physics/SphereColliderComponent.h"

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
    fields.push_back({"Invisible time range", InspectorField::FieldType::Vec2, &invisibleTimeRange, 0.0f, 10.0f});
    fields.push_back({"Attack Angular Speed", InspectorField::FieldType::Float, &attackAngularSpeed, 0.0f, 10.0f});
    fields.push_back({"Main scream duration", InspectorField::FieldType::Float, &mainScreamDuration, 0.1f, 10.0f});
    fields.push_back({"Warning duration", InspectorField::FieldType::Float, &warningDuration, 0.1f, 10.0f});
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
        else if (currentGO->GetName() == "VFX_Banshee_shoutBase")
        {
            shoutBaseComponents = currentGO->GetAllComponentsInChilds<ShaderScriptComponent*>(AppEngine);

            for (ShaderScriptComponent* shaderComponent : shoutBaseComponents)
            {
                shaderComponent->SetScriptEnabled("MovingUVTransparent", false);
            }
        }
        else if (currentGO->GetName() == "VFX_Banshee_shoutStart")
        {
            shoutStartComponents = currentGO->GetAllComponentsInChilds<ShaderScriptComponent*>(AppEngine);

            for (ShaderScriptComponent* shaderComponent : shoutStartComponents)
            {
                shaderComponent->SetScriptEnabled("MovingUVTransparent", false);
            }

            // Take warning star mesh and control mesh scale over time for anim
            std::vector<MeshComponent*> shoutStartMeshes =
                currentGO->GetAllComponentsInChilds<MeshComponent*>(AppEngine);

            for (MeshComponent* currentMesh : shoutStartMeshes)
            {
                if (currentMesh->GetParent()->GetName() == "WarningStar")
                {
                    meshWarningStar = currentMesh;
                    meshWarningStar->SetEnabled(false);
                }
            }
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
        for (ShaderScriptComponent* shaderComponent : shoutStartComponents)
        {
            shaderComponent->SetScriptEnabled("MovingUVTransparent", false);
        }

        for (ShaderScriptComponent* shaderComponent : shoutStartComponents)
        {
            shaderComponent->ResetScript("MovingUVTransparent");
        }

        for (ShaderScriptComponent* shaderComponent : shoutBaseComponents)
        {
            shaderComponent->SetScriptEnabled("MovingUVTransparent", false);
        }

        for (ShaderScriptComponent* shaderComponent : shoutBaseComponents)
        {
            shaderComponent->ResetScript("MovingUVTransparent");
        }

        if (meshWarningStar) meshWarningStar->SetEnabled(false);

        weapon->SetEnabled(false);

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
        if (attackCdTimer <= 0) Attack(deltaTime);
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

    default:
        currentState = BansheeStates::Idle;
        ChangeState();
        break;
    }
}

void Banshee::ChasePlayer()
{
    if (!character) return;

    hasMoved = true;
    if (animComponent) animComponent->UseTrigger("Chase");
    // if (animComponent && animComponent->GetCurrentStateName() != HashString("Idle"))
    // animComponent->UseTrigger("Chase");
    if (CheckDistanceWithPlayer() <= PlayerDistances::Close) currentState = BansheeStates::Attack;
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
        if (animComponent->GetCurrentStateName() == HashString("Teleport") && animComponent->IsFinished())
        {
            currentInvisibleTime = invisibleDist(rng);
            isInvisible          = true;
            mesh->SetEnabled(false);
        }
        if (attackTimer < currentInvisibleTime) return;

        if (isInvisible)
        {
            // Tp to player and enable
            GoToAttackPosition();
            mesh->SetEnabled(true);
            isInvisible = false;
            agentAI->SetAngularSpeed(attackAngularSpeed);
            animComponent->UseTrigger("ScreamIn");

            elapsedWarning = 0.f;
            if (meshWarningStar) meshWarningStar->SetEnabled(true);
        }

        // Slowly rotate towards player while charging the attack
        else if (animComponent->GetCurrentStateName() == HashString("ScreamIn") && !animComponent->IsFinished())
        {
            agentAI->LookAtMovement(character->GetLastPosition(), deltaTime);

            float3 translation, scale;
            Quat rotation;

            meshWarningStar->GetParent()->GetLocalTransform().Decompose(translation, rotation, scale);

            float interpolationValue = min(elapsedWarning / warningDuration, 1.f);

            float finalScale         = Interpolation::Lerp(1.f, 0.f, interpolationValue);
            scale                    = float3(finalScale, 1.f, finalScale);

            float4x4 starTransform   = float4x4::FromTRS(translation, rotation, scale);
            meshWarningStar->GetParent()->SetLocalTransform(starTransform);

            if (elapsedWarning < warningDuration) elapsedWarning += deltaTime;
            else
            {
                float4x4 starTransform = float4x4::FromTRS(translation, rotation, float3::one);
                meshWarningStar->GetParent()->SetLocalTransform(starTransform);
                if (meshWarningStar) meshWarningStar->SetEnabled(false);

                for (ShaderScriptComponent* shaderComponent : shoutStartComponents)
                {
                    shaderComponent->SetScriptEnabled("MovingUVTransparent", true);
                }
            }
        }

        else if (animComponent->GetCurrentStateName() == HashString("ScreamIn") && animComponent->IsFinished())
        {
            animComponent->UseTrigger("Scream");

            weapon->SetEnabled(true);

            for (ShaderScriptComponent* shaderComponent : shoutStartComponents)
            {
                shaderComponent->SetScriptEnabled("MovingUVTransparent", false);
            }

            for (ShaderScriptComponent* shaderComponent : shoutStartComponents)
            {
                shaderComponent->ResetScript("MovingUVTransparent");
            }

            for (ShaderScriptComponent* shaderComponent : shoutBaseComponents)
            {
                shaderComponent->SetScriptEnabled("MovingUVTransparent", true);
            }
        }

        else if (animComponent->GetCurrentStateName() == HashString("Scream") && elapsedMainScream < mainScreamDuration)
            elapsedMainScream += deltaTime;

        else if (animComponent->GetCurrentStateName() == HashString("Scream") &&
                 elapsedMainScream >= mainScreamDuration)
        {
            animComponent->UseTrigger("ScreamOut");

            weapon->SetEnabled(false);

            elapsedMainScream = 0.f;

            for (ShaderScriptComponent* shaderComponent : shoutBaseComponents)
            {
                shaderComponent->SetScriptEnabled("MovingUVTransparent", false);
            }

            for (ShaderScriptComponent* shaderComponent : shoutBaseComponents)
            {
                shaderComponent->ResetScript("MovingUVTransparent");
            }
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
    // const HashString& playerLocation = AppEngine->GetSceneModule()->GetScene()->GetPlayerLocation();
    // bool playerInLocation            = parent->HasTag(playerLocation);

    const float distance = GetDistanceFromPlayer();
    if (distance <= rangeAIAttack) currentState = BansheeStates::Attack;
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

void Banshee::TakeDamage(int amount)
{
    if (isInvisible) return;

    switch (currentState)
    {
    case BansheeStates::Attack:

        if (animComponent->GetCurrentStateName() == HashString("ScreamIn") ||
            animComponent->GetCurrentStateName() == HashString("Teleport"))
        {
            animComponent->UseTrigger("Hit");
            currentState = BansheeStates::Hit;

            for (ShaderScriptComponent* shaderComponent : shoutStartComponents)
            {
                shaderComponent->SetScriptEnabled("MovingUVTransparent", false);
            }

            for (ShaderScriptComponent* shaderComponent : shoutStartComponents)
            {
                shaderComponent->ResetScript("MovingUVTransparent");
            }

            if (meshWarningStar) meshWarningStar->SetEnabled(false);

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
    case BansheeStates::Dead:
        break;
    default:
        isSearching = false;
        animComponent->UseTrigger("Hit");
        currentState = BansheeStates::Hit;
        agentAI->PauseMovement();
        break;
    }

    if ((currentHealth - amount) <= 0)
    {
        animComponent->UseTrigger("Death");
        currentState = BansheeStates::Dead;
        return;
    }

    Character::TakeDamage(amount);
}