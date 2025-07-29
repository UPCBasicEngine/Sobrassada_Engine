#include "pch.h"

#include "Banshee.h"

#include "CuChulainn.h"
#include "DebugDrawModule.h"
#include "GameObject.h"
#include "GameTimer.h"
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
            weapon     = currentGO;
            damageArea = weapon->GetComponent<SphereColliderComponent*>();

            if (damageArea == nullptr) GLOG("[ERROR BANSHEE SCRIPT]: Sphere collider not found")
            else damageArea->SetEnabled(false);

            weapon->SetEnabled(false);
        }
        else if (currentGO->GetName() == "SphereShadow")
        {
            areaVisual = currentGO;
            if (areaVisual) areaVisual->SetEnabled(false);
            else GLOG("[ERROR BANSHEE SCRIPT]: No area visual found")
        }
        else if (currentGO->GetName() == "ScreamVisual")
        {
            screamVisual = currentGO;
            if (screamVisual) screamVisual->SetEnabled(false);
            else GLOG("[ERROR BANSHEE SCRIPT]: No scream visual found")
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
        const std::string life      = "Health: " + std::to_string(currentHealth);
        const std::string animState = "Anim state: " + stateName.GetString();

        std::vector<std::pair<std::string, float2>> logs {
            {life,      float2(-50.0f, -140.0f)},
            {animState, float2(-80.0f, -160.0f)},
        };

        RenderDebug(logs, float3(1.0f, 0.0f, 0.0f));
    }
}

void Banshee::OnPlayerExitLocation()
{
    currentState = BansheeStates::Idle;
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
    }

    if (animComponent && animComponent->IsFinished())
    {
        animComponent->UseTrigger("Idle");
    }
}

void Banshee::ChasePlayer()
{
    if (!character) return;

    if (animComponent) animComponent->UseTrigger("Chase");
    if (CheckDistanceWithPlayer() <= PlayerDistances::Close) currentState = BansheeStates::Attack;
    else if (!agentAI->SetPathNavigation(character->GetLastPosition()) || GetDistanceFromPlayer() > maxDetectionRange)
        currentState = BansheeStates::Search;
}

void Banshee::Attack(float deltaTime)
{
    if (!damageArea) return;

    if (!isAttacking)
    {
        // GLOG("Banshee attack");
        agentAI->SetLookForward(false);

        Character::Attack(deltaTime);
        agentAI->SetSpeed(0.0f, 0.0f);

        currentInvisibleTime = invisibleDist(rng);
        isInvisible          = true;
        mesh->SetEnabled(false);
    }
    else
    {
        if (attackTimer < currentInvisibleTime) return;

        if (isInvisible)
        {
            // Tp to player and enable
            GoToAttackPosition();
            mesh->SetEnabled(true);
            isInvisible = false;
            agentAI->SetAngularSpeed(attackAngularSpeed);
            if (animComponent) animComponent->UseTrigger("Scream");
        }

        // Slowly rotate towards player while charging the attack
        if (attackTimer < currentInvisibleTime + attackHitboxDelay)
            agentAI->LookAtMovement(character->GetLastPosition(), deltaTime);

        if (!damageArea->GetEnabled() && attackTimer >= currentInvisibleTime + attackHitboxDelay &&
            attackTimer <= currentInvisibleTime + attackHitboxDelay + attackHitboxDuration)
        {
            // GLOG("Banshee enable hitbox");
            if (areaVisual) areaVisual->SetEnabled(true);
            if (screamVisual) screamVisual->SetEnabled(true);
            damageArea->SetEnabled(true);
            if (weaponCollider) weaponCollider->SetEnabled(true);
        }
        else if (damageArea->GetEnabled() &&
                 attackTimer >= currentInvisibleTime + attackHitboxDelay + attackHitboxDuration)
        {
            // GLOG("Banshee disable hitbox");
            damageArea->SetEnabled(false);
            if (weaponCollider) weaponCollider->SetEnabled(false);
            if (areaVisual) areaVisual->SetEnabled(false);
            if (screamVisual) screamVisual->SetEnabled(false);
        }

        if (attackTimer >= currentInvisibleTime + attackDuration)
        {
            isAttacking   = false;
            attackCdTimer = attackCooldown;
            agentAI->ResetSpeed();
            agentAI->ResetAngularSpeed();
            agentAI->SetLookForward(true);
            if (GetDistanceFromPlayer() > maxDetectionRange) currentState = BansheeStates::Search;
            else ChangeState();
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
    const HashString& playerLocation = AppEngine->GetSceneModule()->GetScene()->GetPlayerLocation();
    bool playerInLocation            = parent->HasTag(playerLocation);

    const float distance             = GetDistanceFromPlayer();
    if (distance <= rangeAIAttack && playerInLocation) currentState = BansheeStates::Attack;
    else if (distance <= rangeAIChase && playerInLocation) currentState = BansheeStates::Chase;
    else currentState = BansheeStates::Search;
}

void Banshee::SearchForPlayer()
{
    // Stands still for a few seconds, if player gets close again chases, if not returns to patrol
    if (!isSearching)
    {
        // TODO: Would be nice to be a "search" animation instead of idle
        animComponent->UseTrigger("Idle");
        isSearching = true;
        searchTimer = searchDuration;
        agentAI->SetSpeed(0.0f, 0.0f);
    }

    if (GetDistanceFromPlayer() < maxDetectionRange - 0.5f)
    {
        isSearching = false;
        agentAI->ResetSpeed();
        currentState = BansheeStates::Chase;
    }
    else if (searchTimer >= 0.1f && searchTimer <= 0.3f)
    {
        mesh->SetEnabled(false);
        isInvisible = true;
        agentAI->ResetSpeed();
        agentAI->SetPosition(startPos);
    }
    else if (searchTimer <= 0.1f)
    {
        isSearching  = false;
        currentState = BansheeStates::Idle;
        mesh->SetEnabled(true);
        isInvisible = false;
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

void Banshee::OnCollision(GameObject* otherObject, const float3 collisionNormal, ColliderLayer layer)
{
    if (isInvisible) return;

    Character::OnCollision(otherObject, collisionNormal, layer);
}