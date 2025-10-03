#include "pch.h"

#include "Application.h"
#include "AttackVfxSpritesheet.h"
#include "Component.h"
#include "CuChulainn.h"
#include "DebugDrawModule.h"
#include "GameObject.h"
#include "GameTimer.h"
#include "Globals.h"
#include "LibraryModule.h"
#include "ResourceAnimation.h"
#include "ResourceStateMachine.h"
#include "ShaderScriptComponent.h"
#include "Soldier.h"
#include "Standalone/AIAgentComponent.h"
#include "Standalone/AnimationComponent.h"
#include "Standalone/Audio/AudioSourceComponent.h"
#include "Standalone/CharacterControllerComponent.h"
#include "Standalone/MeshComponent.h"
#include "Standalone/Physics/CapsuleColliderComponent.h"

#include "Wwise_IDs.h"
#include <random>

Soldier::Soldier(GameObject* parent)
    : Character(parent, 3, 1, 0.5f, 1.0f, 1.0f, 2.0f, 10.0f, 15.0f, CharacterType::Soldier)
{
    fields.push_back({"AI Patrol Point", InspectorField::FieldType::Vec3, &patrolPoint, -1000.0f, 1000.0f});
    fields.push_back({"Knockback Time", InspectorField::FieldType::Float, &knockbackTime, 0.0f, 1.0f});
    fields.push_back({"Knockback Force", InspectorField::FieldType::Float, &knockbackForce, 0.0f, 20.0f});
    fields.push_back({"Second Attack Delay", InspectorField::FieldType::Float, &secondAttackDelay, 0.0f, 1.0f});
    fields.push_back({"Chase Speed", InspectorField::FieldType::Float, &chaseSpeed, 0.0f, 10.0f});
    fields.push_back({"Cheering distance", InspectorField::FieldType::Float, &cheeringDistance, 0.0f, 10.0f});
    fields.push_back({"Max number of enemies nearby", InspectorField::FieldType::Int, &maxEnemiesNearby, 0, 10});
    fields.push_back({"Melee trail object", InspectorField::FieldType::InputText, &meleeTrailName});
    fields.push_back({"Helmet 1 object", InspectorField::FieldType::InputText, &helmet1Name});
    fields.push_back({"Helmet 2 object", InspectorField::FieldType::InputText, &helmet2Name});
    fields.push_back({"Helmet 3 object", InspectorField::FieldType::InputText, &helmet3Name});
    fields.push_back({"Helmet 4 object", InspectorField::FieldType::InputText, &helmet4Name});
    fields.push_back({"Melee VFX object", InspectorField::FieldType::InputText, &meleeVfxName});
    fields.push_back({"Melee 2 VFX object", InspectorField::FieldType::InputText, &melee2VfxName});
    fields.push_back({"Thrust VFX object", InspectorField::FieldType::InputText, &thrustVfxName});
    fields.push_back({"Alternate material name", InspectorField::FieldType::InputText, &materialName});
}

bool Soldier::Init()
{
    // GLOG("Initiating Soldier");

    currentState = SoldierStates::PATROL;

    Character::Init();

    agentAI = parent->GetComponent<AIAgentComponent*>();
    if (agentAI == nullptr) GLOG("AIAgent component not found for Soldier")
    else
    {
        agentAI->RecreateAgent();
        agentAI->SetLookForward(true);
        speed = agentAI->GetSpeed();
    }

    originalAttackDuration    = attackDuration;
    originalAttackHitboxDelay = attackHitboxDelay;

    meleeTrailObject          = parent->GetChildGameObjectByName(meleeTrailName);
    if (!meleeTrailObject) GLOG("[WARNING] No melee trail found for melee attack in Soldier")
    else
    {
        GLOG("Melee trail found for melee attack in Soldier")
        meleeTrailObject->SetEnabled(false);
    }

    helmet1Object = parent->GetChildGameObjectByName(helmet1Name);
    if (!helmet1Object) GLOG("[WARNING] No helmet 1 found for Soldier")
    else
    {
        GLOG("Helmet 1 found in Soldier")
        helmet1Object->SetEnabled(false);
    }

    helmet2Object = parent->GetChildGameObjectByName(helmet2Name);
    if (!helmet2Object) GLOG("[WARNING] No helmet 2 found for Soldier")
    else
    {
        GLOG("Helmet 2 found in Soldier")
        helmet2Object->SetEnabled(false);
    }

    helmet3Object = parent->GetChildGameObjectByName(helmet3Name);
    if (!helmet3Object) GLOG("[WARNING] No helmet 3 found for Soldier")
    else
    {
        GLOG("Helmet 3 found in Soldier")
        helmet3Object->SetEnabled(false);
    }

    helmet4Object = parent->GetChildGameObjectByName(helmet4Name);
    if (!helmet4Object) GLOG("[WARNING] No helmet 4 found for  Soldier")
    else
    {
        GLOG("Helmet 4 found in Soldier")
        helmet4Object->SetEnabled(false);
    }

    meleeVfxObject = parent->GetChildGameObjectByName(meleeVfxName);
    if (!meleeVfxObject) GLOG("[WARNING] No melee VFX found for melee attack in Soldier")
    else
    {
        GLOG("MeleVFX found in Soldier")
        meleeVfxObject->SetEnabled(false);
    }

    melee2VfxObject = parent->GetChildGameObjectByName(melee2VfxName);
    if (!melee2VfxObject) GLOG("[WARNING] No melee VFX found for melee attack in Soldier")
    else
    {
        GLOG("MeleVFX found in Soldier")
        melee2VfxObject->SetEnabled(false);
    }

    thrustVfxObject = parent->GetChildGameObjectByName(thrustVfxName);
    if (!thrustVfxObject) GLOG("[WARNING] No melee VFX found for melee attack in Soldier")
    else
    {
        GLOG("MeleVFX found in Soldier")
        thrustVfxObject->SetEnabled(false);
    }

    SelectRandomHelmet();

    audio = parent->GetComponent<AudioSourceComponent*>();
    if (!audio) GLOG("[WARNING] Soldier: No audio component found");

    return true;
}

void Soldier::Update(float deltaTime)
{
    if (currentState == SoldierStates::DEATH && animComponent && animComponent->IsFinished())
    {
        parent->SetEnabled(false);
    }

    if (currentState == SoldierStates::DEATH || agentAI == nullptr) return;

    if (isKnockback)
    {
        knockbackTimer           -= deltaTime;

        const float appliedForce  = isStrongKnockback ? knockbackForce * 2 : knockbackForce;
        agentAI->MoveTo(appliedForce, knockbackDirection);
        if (knockbackTimer <= 0.0f)
        {
            isKnockback       = false;
            isStrongKnockback = false;
            agentAI->ResetSpeed();
            agentAI->ResetAngularSpeed();
            ChangeState();
        }
        return;
    }

    if (meleeVfxObject && meleeVfxObject->IsEnabled())
    {
        float duration = animComponent->GetCurrentAnimation()->GetDuration();
    }

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

void Soldier::OnPlayerExitLocation()
{
    currentState = SoldierStates::PATROL;
    agentAI->SetPathNavigation(startPos);
    reachedPatrolPoint = false;
}

void Soldier::OnPlayerEnterLocation()
{
    currentState = SoldierStates::PATROL;
    if (agentAI) agentAI->SetPathNavigation(startPos);
    reachedPatrolPoint = false;
}

void Soldier::SetAttackVFX(GameObject* selectedMeleeVfxObject)
{

    if (selectedMeleeVfxObject && !selectedMeleeVfxObject->IsEnabled())
    {
        selectedMeleeVfxObject->SetEnabled(true);
        selectedMeleeVfxObject->GetComponent<MeshComponent*>()->SetEnabled(false);
        selectedMeleeVfxObject->GetComponent<ShaderScriptComponent*>()->GetScriptByType<AttackVfxSpritesheet>()->Reset(
        );
    }
}

void Soldier::DisableAttackVFX()
{
    if (meleeVfxObject) meleeVfxObject->SetEnabled(false);
}

void Soldier::OnDeath()
{
    // TODO: include death sound for the character
    // TODO: animation and particles
    isAttacking = false;
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(1, 2);
    if (dis(gen) == 1)
    {
        if (animComponent) animComponent->UseTrigger("death");
    }   
    else
    {
        if (animComponent) animComponent->UseTrigger("death2");
    }
    
    audio->EmitEvent(AK::EVENTS::PLAY_SFX_SOLDIER_DEATH);
    agentAI->PauseMovement();
    currentState = SoldierStates::DEATH;
    playerScript->RemoveEnemy();
    countedInPlayerEnemies = false;
}

void Soldier::OnDamageTaken(int amount)
{
    isAttacking = false;
    attackTimer = 0.0f;
    if (weaponCollider && weaponCollider->GetEnabled())
    {
        weaponCollider->SetEnabled(false);
    }
    isKnockback    = true;
    knockbackTimer = knockbackTime;

    isStrongKnockback =
        (playerScript &&
         (playerScript->GetState() == CharacterStates::HEAL || playerScript->GetState() == CharacterStates::TRANSFORM));
    if (currentHealth != 0) audio->EmitEvent(AK::EVENTS::PLAY_SFX_SOLDIER_HURT);
    ApplyKnockback();
    // HashString animStateFromPlayer = GetAnimStateNameFromPlayer();
    // std::string animState               = animStateFromPlayer.GetString();
    // GLOG("Soldier %s damaged with state %s", parent->GetName().c_str(), animState.c_str());
    if (animComponent) animComponent->UseTrigger("damaged");
    if (meleeTrailObject) meleeTrailObject->SetEnabled(false);
}

void Soldier::PerformAttack()
{
    // TODO: play basicAttack sound
    // TODO: make interaction with hitboxes with the character
    // TODO: activate and disable the box collider located on one on the gameobjects weapon
    // TODO: trails, particles and animation
}

void Soldier::HandleState(float deltaTime)
{
    if (!animComponent) return;

    switch (currentState)
    {
    case SoldierStates::SEARCH:
        SearchForPlayer();
        break;
    case SoldierStates::PATROL:
        agentAI->ResetSpeed();
        PatrolAI(deltaTime);
        // TODO: patrol animation
        animComponent->UseTrigger("patrol");
        break;
    case SoldierStates::CHASE:
        agentAI->LookAtMovement(character->GetLastPosition(), deltaTime);
        animComponent->UseTrigger("run");
        agentAI->SetSpeed(chaseSpeed, 8.0);
        // GLOG("Speed set to %f", patrolSpeed);
        ChaseAI();
        break;
    case SoldierStates::BASIC_ATTACK:
        agentAI->ResumeMovement();
        if (attackCdTimer <= 0) Attack(deltaTime);
        break;
    case SoldierStates::PLAYER_DETECTION:
        animComponent->UseTrigger("detectPlayer");
        if (!detectAudioPlayed)
        {
            audio->EmitEvent(AK::EVENTS::PLAY_SFX_SOLDIER_DETECT);
            detectAudioPlayed = true;
        }
        if (animComponent->IsFinished())
        {
            currentState = SoldierStates::CHASE;
            detectAudioPlayed = false;
        }
        break;
    case SoldierStates::CHEERING:
        agentAI->LookAtMovement(character->GetLastPosition(), deltaTime);
        if (playerScript->GetEnemiesCount() < maxEnemiesNearby)
        {
            agentAI->ResetSpeed();
            ChangeState();
        }
        break;
    default:
        GLOG("No state provided to Soldier");
        currentState = SoldierStates::PATROL;
        break;
    }

    if (animComponent && animComponent->IsFinished())
    {
        if (currentState == SoldierStates::BASIC_ATTACK) animComponent->UseTrigger("idleCombat");
        else animComponent->UseTrigger("idle");
    }
}

void Soldier::PatrolAI(float deltaTime)
{
    const HashString& playerLocation = AppEngine->GetSceneModule()->GetScene()->GetPlayerLocation();
    // GLOG("Player location: %s", playerLocation.GetString().c_str());
    bool playerInLocation            = parent->HasTag(playerLocation);

    if (!playerScript->IsDead())
    {
        if (CheckDistanceWithPlayer() == PlayerDistances::Medium && playerInLocation)
        {
            currentState = SoldierStates::PLAYER_DETECTION;
        }
        else if (CheckDistanceWithPlayer() == PlayerDistances::Close && playerInLocation)
            currentState = SoldierStates::BASIC_ATTACK;
    }

    bool valid = false;
    if (reachedPatrolPoint)
    {
        if (CheckDistanceWithPoint(startPos)) reachedPatrolPoint = false;
        else
        {
            valid = agentAI->SetPathNavigation(startPos);
            agentAI->LookAtMovement(startPos, deltaTime);
        }
    }
    else
    {
        if (CheckDistanceWithPoint(patrolPoint)) reachedPatrolPoint = true;
        else
        {
            valid = agentAI->SetPathNavigation(patrolPoint);
            agentAI->LookAtMovement(patrolPoint, deltaTime);
        }
    }
}

void Soldier::ChaseAI()
{
    if (character != nullptr)
    {
        agentAI->SetPathNavigation(character->GetLastPosition());
        ChangeState();
    }
    else currentState = SoldierStates::PATROL;
}

void Soldier::SearchForPlayer()
{
    // GLOG("Searching for player");
    //  Stands still for a few seconds, if player gets close again chases, if not returns to patrol
    if (!isSearching)
    {
        // TODO: Would be nice to be a "search" animation instead of idle
        animComponent->UseTrigger("search");
        isSearching = true;
        searchTimer = searchDuration;
        agentAI->SetSpeed(0.0f, 10.0f);
    }

    if (GetDistanceFromPlayer() < maxDetectionRange - 0.5f)
    {
        isSearching = false;
        agentAI->ResetSpeed();
        currentState = SoldierStates::PLAYER_DETECTION;
    }
    else if (searchTimer <= 0.0f)
    {
        isSearching = false;
        agentAI->SetSpeed(chaseSpeed, 8.0);
        GLOG("Speed set to %f", chaseSpeed);
        currentState = SoldierStates::PATROL;
    }
}

void Soldier::Attack(float deltaTime)
{
    if (!weaponCollider) return;

    if (!isAttacking)
    {
        GLOG("ATTACK ENEMY");
        if (animComponent)
        {
            attackHitboxDelay    = originalAttackHitboxDelay;
            currentAttackTrigger = ManageAttackAnimations();

            if (currentAttackTrigger && strcmp(currentAttackTrigger, "attack") == 0)
            {
                attackHitboxDelay += 0.4f;
                attackDuration     = attackHitboxDelay + 2 * attackHitboxDuration + secondAttackDelay + 0.1f;
            }
            else
            {
                attackDuration = originalAttackDuration;
            }
        }
        Character::Attack(deltaTime);
        //agentAI->PauseMovement();
        thrustAdvance = false;
    }
    else
    {
        if (meleeTrailObject) meleeTrailObject->SetEnabled(true);
        //agentAI->ResumeMovement();
        agentAI->LookAtMovement(character->GetLastPosition(), deltaTime);
        // Doble attack
        if (currentAttackTrigger && strcmp(currentAttackTrigger, "attack") == 0)
        {
            bool inFirstWindow =
                attackTimer >= attackHitboxDelay && attackTimer <= attackHitboxDelay + attackHitboxDuration;
            float secondDelay   = attackHitboxDelay + attackHitboxDuration + secondAttackDelay;
            bool inSecondWindow = attackTimer >= secondDelay && attackTimer <= secondDelay + attackHitboxDuration;

            if ((inFirstWindow || inSecondWindow) && !weaponCollider->GetEnabled())
            {
                if (meleeVfxObject && !meleeVfxObject->IsEnabled())
                {
                    meleeVfxOriginalTransform = meleeVfxObject->GetLocalTransform();
                }
                weaponCollider->SetEnabled(true);
                if (inFirstWindow && audio) audio->EmitEvent(AK::EVENTS::PLAY_SFX_SOLDIER_SLASH_1);
                if (inSecondWindow && audio) audio->EmitEvent(AK::EVENTS::PLAY_SFX_SOLDIER_SLASH_2);
                if (inFirstWindow)
                {
                    GLOG("First window is true");
                    SetAttackVFX(meleeVfxObject);
                }
                if (inSecondWindow)
                {
                    SetAttackVFX(melee2VfxObject);
                }
            }
            else if (!inFirstWindow && !inSecondWindow)
            {
                if (weaponCollider->GetEnabled()) weaponCollider->SetEnabled(false);
                if (meleeVfxObject && meleeVfxObject->IsEnabled())
                {
                    meleeVfxObject->SetEnabled(false);
                }
                if (melee2VfxObject && melee2VfxObject->IsEnabled())
                {
                    melee2VfxObject->SetEnabled(false);
                }
            }
        }
        else // thrust
        {
            if (thrustVfxObject && !thrustVfxObject->IsEnabled())
            {
                thrustVfxObject->SetEnabled(true);
                thrustVfxObject->GetComponent<MeshComponent*>()->SetEnabled(false);
                thrustVfxObject->GetComponent<ShaderScriptComponent*>()->GetScriptByType<AttackVfxSpritesheet>()->Reset(
                );
            }

            if (!weaponCollider->GetEnabled() && attackTimer >= attackHitboxDelay &&
                attackTimer <= attackHitboxDelay + attackHitboxDuration)
            {
                weaponCollider->SetEnabled(true);
                if (audio) audio->EmitEvent(AK::EVENTS::PLAY_SFX_SOLDIER_THRUST);

                thrustAdvance = true;
            }
            else if (weaponCollider->GetEnabled() && attackTimer >= attackHitboxDelay + attackHitboxDuration)
            {
                weaponCollider->SetEnabled(false);
            }

            if (animComponent && !animComponent->IsFinished() && thrustAdvance)
            {
                /*agentAI->PauseMovement();*/
                float thrustSpeed = 2.0f;
                float3 forward    = parent->GetGlobalTransform().WorldZ();
                forward.y         = parent->GetGlobalTransform().WorldY().y;
                forward.Normalize();
                agentAI->SetPosition(parent->GetGlobalTransform().TranslatePart() + forward * thrustSpeed * deltaTime);
            }
        }

        // Reset attack state
        if (attackTimer >= attackDuration)
        {
            //agentAI->ResumeMovement();
            agentAI->LookAtMovement(character->GetLastPosition(), deltaTime);
            isAttacking   = false;
            attackCdTimer = attackCooldown;
            if (meleeTrailObject) meleeTrailObject->SetEnabled(false);
            if (meleeVfxObject) meleeVfxObject->SetEnabled(false);
            if (thrustVfxObject) thrustVfxObject->SetEnabled(false);
            ChangeState();
        }
    }
}

void Soldier::ChangeState()
{
    if (playerScript->IsDead())
    {
        currentState = SoldierStates::PATROL;
        return;
    }

    const float distance = GetDistanceFromPlayer();

    if (distance <= cheeringDistance)
    {
        if (playerScript->GetEnemiesCount() >= maxEnemiesNearby)
        {
            if (!countedInPlayerEnemies)
            {
                SetOnWaiting();
                return;
            }
        }
        else
        {
            if (!countedInPlayerEnemies)
            {
                playerScript->AddEnemy();
                countedInPlayerEnemies = true;
                GLOG("Enemy entered. Total unique enemies colliding: %zu", playerScript->GetEnemiesCount());
            }
        }
    }
    else
    {
        if (countedInPlayerEnemies)
        {
            playerScript->RemoveEnemy();
            countedInPlayerEnemies = false;
        }
    }

    if (distance <= rangeAIAttack)
    {
        agentAI->PauseMovement();
        currentState = SoldierStates::BASIC_ATTACK;
        animComponent->UseTrigger("idleCombat");
        
    }
    else if (distance <= rangeAIChase) currentState = SoldierStates::CHASE;
    else if (distance > maxDetectionRange) currentState = SoldierStates::SEARCH;
}

void Soldier::ApplyKnockback()
{
    const float3 myPos   = parent->GetGlobalTransform().TranslatePart();
    const float3 origin  = character ? character->GetLastPosition() : float3::zero;

    knockbackDirection   = myPos - origin;
    knockbackDirection.y = 0.0f;
    if (knockbackDirection.LengthSq() < 0.001f) knockbackDirection = float3::unitZ;
    knockbackDirection.Normalize();
}

const char* Soldier::ManageAttackAnimations()
{
    const char* attackTrigger = nullptr;
    if (consecutiveAttack >= 2)
    {
        attackTrigger     = "thrust";
        consecutiveThrust = 1;
        consecutiveAttack = 0;
    }
    else if (consecutiveThrust >= 2)
    {
        attackTrigger     = "attack";
        consecutiveAttack = 1;
        consecutiveThrust = 0;
    }
    else
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<> dis(0, 1);
        bool chooseAttack1 = dis(gen) == 0;

        if (chooseAttack1)
        {
            attackTrigger = "attack";
            consecutiveAttack++;
            consecutiveThrust = 0;
        }
        else
        {
            attackTrigger = "thrust";
            consecutiveThrust++;
            consecutiveAttack = 0;
        }
    }
    animComponent->UseTrigger(attackTrigger);

    return attackTrigger;
}

void Soldier::SetOnWaiting()
{
    GLOG("Soldier %s is waiting", parent->GetName().c_str());
    currentState = SoldierStates::CHEERING;
    agentAI->SetSpeed(0.0f, 10.0f);
    if (animComponent) animComponent->UseTrigger("cheer");
}

void Soldier::SelectRandomHelmet()
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(1, 2);

    if (dis(gen) == 1)
    {
        isRed = true;
    }

    static std::random_device rd2;
    static std::mt19937 gen2(rd2());
    static std::uniform_int_distribution<> dis2(1, 2);

    if (isRed)
    {
        bodyObject = parent->GetChildGameObjectByName("Body");
        if (!bodyObject) GLOG("[WARNING] No melee VFX found for melee attack in Soldier")
        else
        {

            UID materialUID                                     = AppEngine->GetLibraryModule()->GetMaterialUID(materialName);
            MeshComponent* meshComponent = bodyObject->GetComponent<MeshComponent*>();
            if (materialUID) meshComponent->AddMaterial(materialUID);
        }

        switch (dis2(gen2))
        {
        case 1:
            helmet2Object = parent->GetChildGameObjectByName(helmet2Name);
            if (!helmet2Object) GLOG("[WARNING] No helmet 2 found for  Soldier")
            else
            {
                GLOG("Helmet 2 found for in Soldier")
                helmet2Object->SetEnabled(true);
            }
            break;
        case 2:
            helmet3Object = parent->GetChildGameObjectByName(helmet3Name);
            if (!helmet3Object) GLOG("[WARNING] No helmet 3 found for  Soldier")
            else
            {
                GLOG("Helmet 3 found for in Soldier")
                helmet3Object->SetEnabled(true);
            }
            break;
        default:
            break;
        }
    }
    else
    {
        switch (dis2(gen2))
        {
        case 1:
            helmet1Object = parent->GetChildGameObjectByName(helmet1Name);
            if (!helmet1Object) GLOG("[WARNING] No helmet 1 found for  Soldier")
            else
            {
                GLOG("Helmet 1 found for in Soldier")
                helmet1Object->SetEnabled(true);
            }
            break;
        case 2:
            helmet4Object = parent->GetChildGameObjectByName(helmet4Name);
            if (!helmet4Object) GLOG("[WARNING] No helmet 4 found for  Soldier")
            else
            {
                GLOG("Helmet 4 found for in Soldier")
                helmet4Object->SetEnabled(true);
            }
            break;
        default:
            break;
        }
    }

    switch (dis2(gen2))
    {
    case 1:
        helmet1Object = parent->GetChildGameObjectByName(helmet1Name);
        if (!helmet1Object) GLOG("[WARNING] No helmet 1 found for  Soldier")
        else
        {
            GLOG("Helmet 1 found for in Soldier")
            helmet1Object->SetEnabled(true);
        }
        break;
    case 2:
        helmet2Object = parent->GetChildGameObjectByName(helmet2Name);
        if (!helmet2Object) GLOG("[WARNING] No helmet 2 found for  Soldier")
        else
        {
            GLOG("Helmet 2 found for in Soldier")
            helmet2Object->SetEnabled(true);
        }
        break;
    case 3:
        helmet3Object = parent->GetChildGameObjectByName(helmet3Name);
        if (!helmet3Object) GLOG("[WARNING] No helmet 3 found for  Soldier")
        else
        {
            GLOG("Helmet 3 found for in Soldier")
            helmet3Object->SetEnabled(true);
        }
        break;
    case 4:
        helmet4Object = parent->GetChildGameObjectByName(helmet4Name);
        if (!helmet4Object) GLOG("[WARNING] No helmet 4 found for  Soldier")
        else
        {
            GLOG("Helmet 4 found for in Soldier")
            helmet4Object->SetEnabled(true);
        }
        break;
    default:
        break;
    }
}
