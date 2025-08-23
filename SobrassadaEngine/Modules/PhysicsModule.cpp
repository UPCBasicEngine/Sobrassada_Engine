#include "PhysicsModule.h"

#include "Application.h"
#include "BulletDebugDraw.h"
#include "DebugDrawModule.h"
#include "GameObject.h"
#include "GameTimer.h"
#include "SceneModule.h"
#include "Standalone/Physics/CapsuleColliderComponent.h"
#include "Standalone/Physics/CubeColliderComponent.h"
#include "Standalone/Physics/SphereColliderComponent.h"

#include "Math/float3.h"
#include "btBulletDynamicsCommon.h"

PhysicsModule::PhysicsModule()
{
    collisionConfiguration = new btDefaultCollisionConfiguration();
    dispatcher             = new btCollisionDispatcher(collisionConfiguration);
    broadPhase             = new btDbvtBroadphase();
    solver                 = new btSequentialImpulseConstraintSolver;
    dynamicsWorld          = new btDiscreteDynamicsWorld(dispatcher, broadPhase, solver, collisionConfiguration);

    debugDraw              = new BulletDebugDraw();

    debugDraw->setDebugMode(0);

    dynamicsWorld->setGravity(btVector3(0, gravity, 0));
    dynamicsWorld->setDebugDrawer(debugDraw);

    colliderLayerConfig.assign(sizeof(ColliderLayerStrings) / sizeof(char*), LayerBitset().reset());
    LoadLayerData();
}

bool PhysicsModule::Init()
{
    return true;
}

update_status PhysicsModule::PreUpdate(float time)
{
    float deltaTime = App->GetGameTimer()->GetDeltaTime() / 1000.0f;

    // REMOVE RIGID BODIES
    for (btRigidBody* rigidBody : bodiesToRemove)
    {
        // dynamicsWorld->removeCollisionObject(rigidBody);
        dynamicsWorld->removeRigidBody(rigidBody);
        btCollisionShape* shape = rigidBody->getCollisionShape();
        delete shape;
        delete rigidBody;
    }
    bodiesToRemove.clear();

    if (!App->GetSceneModule()->GetInPlayMode() || App->GetSceneModule()->GetScene() == nullptr ||
        !App->GetSceneModule()->GetScene()->isSceneLoaded)
        return UPDATE_CONTINUE;

    if (deltaTime == 0.0f) return UPDATE_CONTINUE;

    if (deltaTime > 0.1f) return UPDATE_CONTINUE; // TODO: deltaTime spikes, need to know why

    dynamicsWorld->stepSimulation(deltaTime, 10);

    // COLLISION CHECKS
    int numManifolds = dynamicsWorld->getDispatcher()->getNumManifolds();
    for (int currentManifold = 0; currentManifold < numManifolds; ++currentManifold)
    {
        // Collision Handler
        const btPersistentManifold* contactManifold =
            dynamicsWorld->getDispatcher()->getManifoldByIndexInternal(currentManifold);

        // CHECKING THAT THERE IS AN ACTUAL COLLISION
        if (contactManifold->getNumContacts() > 0)
        {
            // Getting own data structure to handle callbacks
            BulletUserPointer* firstUserPointer =
                static_cast<BulletUserPointer*>(contactManifold->getBody0()->getUserPointer());
            BulletUserPointer* secondUserPointer =
                static_cast<BulletUserPointer*>(contactManifold->getBody1()->getUserPointer());

            // Calculating normal
            const float3 normal = float3(contactManifold->getContactPoint(0).m_normalWorldOnB);

            UID g1              = firstUserPointer->collider ? firstUserPointer->collider->GetParentUID() : INVALID_UID;
            UID g2 = secondUserPointer->collider ? secondUserPointer->collider->GetParentUID() : INVALID_UID;

            // Still OnCollision
            if (wereColliding.find(g1) != wereColliding.end() && wereColliding.find(g2) != wereColliding.end())
            {
                if (firstUserPointer->generateCallback && firstUserPointer->collider && secondUserPointer->collider)
                    firstUserPointer->onCollisionCallback->Call(
                        secondUserPointer->collider->GetParent(), normal, secondUserPointer->layer
                    );
                if (secondUserPointer->generateCallback && secondUserPointer->collider && firstUserPointer->collider)
                    secondUserPointer->onCollisionCallback->Call(
                        firstUserPointer->collider->GetParent(), -normal, firstUserPointer->layer
                    );
            }
            // First time colliding
            else
            {
                if (firstUserPointer->generateCallback && firstUserPointer->collider && secondUserPointer->collider)
                    firstUserPointer->onCollisionEnterCallback->Call(
                        secondUserPointer->collider->GetParent(), normal, secondUserPointer->layer
                    );
                if (secondUserPointer->generateCallback && secondUserPointer->collider && firstUserPointer->collider)
                    secondUserPointer->onCollisionEnterCallback->Call(
                        firstUserPointer->collider->GetParent(), -normal, firstUserPointer->layer
                    );
            }

            // Adding both elements to areColliding
            if (areColliding.find(g1) != areColliding.end()) areColliding[g1].insert(g2);
            else areColliding.insert({g1, {g2}});

            if (areColliding.find(g2) != areColliding.end()) areColliding[g2].insert(g1);
            else areColliding.insert({g2, {g1}});
        }
    }

    // ON COLLISION EXIT CHECKS
    for (auto& wereCollidingPair : wereColliding)
    {
        UID currentUID = wereCollidingPair.first;
        for (auto& otherUID : wereCollidingPair.second)
        {
            bool exit = false;
            if (areColliding.find(currentUID) == areColliding.end()) exit = true;
            else if (areColliding.find(currentUID) != areColliding.end() &&
                     areColliding[currentUID].find(otherUID) == areColliding[currentUID].end())
                exit = true;

            if (exit && collisionObjects.find(currentUID) != collisionObjects.end() &&
                collisionObjects.find(otherUID) != collisionObjects.end())
            {
                BulletUserPointer firstUserPointer  = collisionObjects[currentUID];
                BulletUserPointer secondUserPointer = collisionObjects[otherUID];

                if (firstUserPointer.generateCallback && firstUserPointer.collider && secondUserPointer.collider)
                    firstUserPointer.onCollisionExitCallback->Call(
                        secondUserPointer.collider->GetParent(), secondUserPointer.layer
                    );
            }
        }
    }

    wereColliding.swap(areColliding);
    areColliding.clear();

    return UPDATE_CONTINUE;
}

update_status PhysicsModule::Render(float deltaTime)
{
    if (debugDraw->getDebugMode()) dynamicsWorld->debugDrawWorld();

    return UPDATE_CONTINUE;
}

update_status PhysicsModule::PostUpdate(float deltaTime)
{
    if (updateGravity)
    {
        updateGravity = false;
        dynamicsWorld->setGravity(btVector3(0, gravity, 0));
    }

    return UPDATE_CONTINUE;
}

bool PhysicsModule::ShutDown()
{
    EmptyWorld();

    delete dynamicsWorld;
    delete solver;
    delete broadPhase;
    delete dispatcher;
    delete collisionConfiguration;
    delete debugDraw;

    return true;
}

void PhysicsModule::CreateCubeRigidBody(CubeColliderComponent* colliderComponent)
{
    // Collision shape
    btCollisionShape* collisionShape = new btBoxShape(btVector3(
        btScalar(colliderComponent->size.x), btScalar(colliderComponent->size.y), btScalar(colliderComponent->size.z)
    ));

    const bool isDynamic             = (colliderComponent->mass != 0.f);

    // Inertia
    btVector3 localInertia(0, 0, 0);
    if (isDynamic) collisionShape->calculateLocalInertia(colliderComponent->mass, localInertia);

    // MotionState for RENDER AND
    colliderComponent->motionState =
        BulletMotionState(colliderComponent, colliderComponent->centerOffset, colliderComponent->centerRotation);

    // Creating final RigidBody
    btRigidBody::btRigidBodyConstructionInfo rbInfo(
        btScalar(colliderComponent->mass), &colliderComponent->motionState, collisionShape, localInertia
    );
    btRigidBody* newRigidBody = new btRigidBody(rbInfo);

    newRigidBody->setUserPointer(&colliderComponent->userPointer);
    collisionObjects[colliderComponent->GetParentUID()] = colliderComponent->userPointer;

    colliderComponent->rigidBody                        = newRigidBody;

    AddRigidBody(newRigidBody, colliderComponent->colliderType, colliderComponent->layer);
}

void PhysicsModule::UpdateCubeRigidBody(CubeColliderComponent* colliderComponent)
{
    DeleteCubeRigidBody(colliderComponent);
    CreateCubeRigidBody(colliderComponent);
}

void PhysicsModule::DeleteCubeRigidBody(CubeColliderComponent* colliderComponent)
{
    if (colliderComponent->rigidBody == nullptr) return;

    collisionObjects.erase(colliderComponent->GetParentUID());
    bodiesToRemove.push_back(colliderComponent->rigidBody);
    colliderComponent->rigidBody = nullptr;
}

void PhysicsModule::CreateSphereRigidBody(SphereColliderComponent* colliderComponent)
{
    // Collision shape
    const float3 scale   = colliderComponent->GetParent()->GetScale();
    const float3& offset = colliderComponent->centerOffset;
    float3 scaledOff {offset.x * scale.x, offset.y * scale.y, offset.z * scale.z};

    btCollisionShape* collisionShape = new btSphereShape(colliderComponent->radius);

    const bool isDynamic             = (colliderComponent->mass != 0.f);

    // Inertia
    btVector3 localInertia(0, 0, 0);
    if (isDynamic) collisionShape->calculateLocalInertia(colliderComponent->mass, localInertia);

    // MotionState for RENDER AND
    colliderComponent->motionState =
        BulletMotionState(colliderComponent, scaledOff, colliderComponent->centerRotation);

    // Creating final RigidBody
    btRigidBody::btRigidBodyConstructionInfo rbInfo(
        btScalar(colliderComponent->mass), &colliderComponent->motionState, collisionShape, localInertia
    );
    btRigidBody* newRigidBody = new btRigidBody(rbInfo);

    newRigidBody->setUserPointer(&colliderComponent->userPointer);
    collisionObjects[colliderComponent->GetParentUID()] = colliderComponent->userPointer;

    colliderComponent->rigidBody                        = newRigidBody;

    AddRigidBody(newRigidBody, colliderComponent->colliderType, colliderComponent->layer);
}

void PhysicsModule::UpdateSphereRigidBody(SphereColliderComponent* colliderComponent)
{
    DeleteSphereRigidBody(colliderComponent);
    CreateSphereRigidBody(colliderComponent);
}

void PhysicsModule::DeleteSphereRigidBody(SphereColliderComponent* colliderComponent)
{
    if (colliderComponent->rigidBody == nullptr) return;

    collisionObjects.erase(colliderComponent->GetParentUID());
    bodiesToRemove.push_back(colliderComponent->rigidBody);
    colliderComponent->rigidBody = nullptr;
}

void PhysicsModule::CreateCapsuleRigidBody(CapsuleColliderComponent* colliderComponent)
{
    // Collision shape
    btCollisionShape* collisionShape = new btCapsuleShape(colliderComponent->radius, colliderComponent->length);

    const bool isDynamic             = (colliderComponent->mass != 0.f);

    // Inertia
    btVector3 localInertia(0.f, 0.f, 0.f);
    if (isDynamic) collisionShape->calculateLocalInertia(colliderComponent->mass, localInertia);

    // MotionState
    colliderComponent->motionState =
        BulletMotionState(colliderComponent, colliderComponent->centerOffset, colliderComponent->centerRotation);

    // Creating final RigidBody
    btRigidBody::btRigidBodyConstructionInfo rbInfo(
        btScalar(colliderComponent->mass), &colliderComponent->motionState, collisionShape, localInertia
    );
    btRigidBody* newRigidBody = new btRigidBody(rbInfo);

    newRigidBody->setUserPointer(&colliderComponent->userPointer);
    collisionObjects[colliderComponent->GetParentUID()] = colliderComponent->userPointer;

    colliderComponent->rigidBody                        = newRigidBody;

    AddRigidBody(newRigidBody, colliderComponent->colliderType, colliderComponent->layer);
}

void PhysicsModule::UpdateCapsuleRigidBody(CapsuleColliderComponent* colliderComponent)
{
    DeleteCapsuleRigidBody(colliderComponent);
    CreateCapsuleRigidBody(colliderComponent);
}

void PhysicsModule::DeleteCapsuleRigidBody(CapsuleColliderComponent* colliderComponent)
{
    if (colliderComponent->rigidBody == nullptr) return;

    collisionObjects.erase(colliderComponent->GetParentUID());
    bodiesToRemove.push_back(colliderComponent->rigidBody);
    colliderComponent->rigidBody = nullptr;
}

void PhysicsModule::SetDebugOption(int option)
{
    debugDraw->setDebugMode(option);
}

void PhysicsModule::AddRigidBody(btRigidBody* rigidBody, ColliderType colliderType, ColliderLayer layerType)
{
    switch (colliderType)
    {
    case ColliderType::DYNAMIC:
        rigidBody->setCollisionFlags(rigidBody->getCollisionFlags() | btCollisionObject::CF_DYNAMIC_OBJECT);
        break;
    case ColliderType::KINEMATIC:
        rigidBody->setCollisionFlags(rigidBody->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
        rigidBody->setActivationState(DISABLE_DEACTIVATION);
        rigidBody->setCustomDebugColor(btVector3(0.859f, 0.196f, 0.784f)); // COLOR TO NOT BE RED, COLORBLIND ISSUE
        break;
    case ColliderType::TRIGGER:
        rigidBody->setCollisionFlags(
            rigidBody->getCollisionFlags() | btCollisionObject::CF_STATIC_OBJECT |
            btCollisionObject::CF_NO_CONTACT_RESPONSE
        );
        break;
    case ColliderType::STATIC:
        rigidBody->setCollisionFlags(rigidBody->getCollisionFlags() | btCollisionObject::CF_STATIC_OBJECT);
        break;
    default:
        break;
    }

    const int group               = 1 << (int)layerType;

    int mask                      = 0;
    const LayerBitset& maskBitset = colliderLayerConfig[(int)layerType];
    for (int i = 0; i < maskBitset.size(); ++i)
    {
        if (maskBitset[i]) mask |= 1 << i;
    }

    dynamicsWorld->addRigidBody(rigidBody, group, mask);
}
// TODO READ FROM CONFIG FILE
void PhysicsModule::LoadLayerData(const rapidjson::Value* initialState)
{
    for (int i = 0; i < colliderLayerConfig.size(); ++i)
        colliderLayerConfig[i].reset();
    // loading defaults if no scene state with saved data
    if (initialState == nullptr)
    {
        // STATIC OBJECT
        int config =
            1 << (int)ColliderLayer::PLAYER | 1 << (int)ColliderLayer::ENEMY | 1 << (int)ColliderLayer::WORLD_OBJECTS;
        colliderLayerConfig[0] |= config;

        // TRIGGER
        config                  = 1 << (int)ColliderLayer::PLAYER;
        colliderLayerConfig[1] |= config;

        // ENEMY
        config                  = 1 << (int)ColliderLayer::PLAYER | 1 << (int)ColliderLayer::WORLD_OBJECTS;
        colliderLayerConfig[2] |= config;

        // PLAYER
        config                  = 1 << (int)ColliderLayer::ENEMY | 1 << (int)ColliderLayer::WORLD_OBJECTS |
                 1 << (int)ColliderLayer::TRIGGERS | 1 << (int)ColliderLayer::ENEMY_PROJECTILE;
        colliderLayerConfig[3] |= config;

        // PLAYER PROJECTILE
        config                  = 1 << (int)ColliderLayer::ENEMY;
        colliderLayerConfig[4] |= config;

        // ENEMY PROJECTILE
        config                  = 1 << (int)ColliderLayer::PLAYER;
        colliderLayerConfig[5] |= config;
    }
    else
    {
        // LOADING LAYER CONFIG IN ORDER OF ColliderLayerStrings[]
        const rapidjson::Value& initialStateRef = *initialState;
        int currentMask                         = 0;

        if (initialStateRef.HasMember("WorldObjectsMask"))
        {
            currentMask             = initialStateRef["WorldObjectsMask"].GetInt();

            colliderLayerConfig[0] |= currentMask;
        }

        if (initialStateRef.HasMember("TriggerMask"))
        {
            currentMask             = initialStateRef["TriggerMask"].GetInt();

            colliderLayerConfig[1] |= currentMask;
        }

        if (initialStateRef.HasMember("EnemyMask"))
        {
            currentMask             = initialStateRef["EnemyMask"].GetInt();

            colliderLayerConfig[2] |= currentMask;
        }

        if (initialStateRef.HasMember("PlayerMask"))
        {
            currentMask             = initialStateRef["PlayerMask"].GetInt();

            colliderLayerConfig[3] |= currentMask;
        }

        if (initialStateRef.HasMember("PlayerProjectileMask"))
        {
            currentMask             = initialStateRef["PlayerProjectileMask"].GetInt();

            colliderLayerConfig[4] |= currentMask;
        }

        if (initialStateRef.HasMember("EnemyProjectileMask"))
        {
            currentMask             = initialStateRef["EnemyProjectileMask"].GetInt();

            colliderLayerConfig[5] |= currentMask;
        }
    }
}

void PhysicsModule::SaveLayerData(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator)
{
    // SAVING LAYER CONFIG IN ORDER OF ColliderLayerStrings[]

    int masks[sizeof(ColliderLayerStrings) / sizeof(char*)] {};

    for (int i = 0; i < colliderLayerConfig.size(); ++i)
    {
        int currentMask = 0;
        for (int j = 0; j < colliderLayerConfig[i].size(); ++j)
        {
            if (colliderLayerConfig[i][j]) currentMask |= 1 << j;
        }
        masks[i] = currentMask;
    }

    targetState.AddMember("WorldObjectsMask", masks[0], allocator);
    targetState.AddMember("TriggerMask", masks[1], allocator);
    targetState.AddMember("EnemyMask", masks[2], allocator);
    targetState.AddMember("PlayerMask", masks[3], allocator);
    targetState.AddMember("PlayerProjectileMask", masks[4], allocator);
    targetState.AddMember("EnemyProjectileMask", masks[5], allocator);
}

void PhysicsModule::EmptyWorld()
{
    // REMOVE RIGID BODIES
    for (btRigidBody* rigidBody : bodiesToRemove)
    {
        // dynamicsWorld->removeCollisionObject(rigidBody);
        dynamicsWorld->removeRigidBody(rigidBody);
        btCollisionShape* shape = rigidBody->getCollisionShape();
        delete shape;
        delete rigidBody;
    }
    bodiesToRemove.clear();

    for (int i = dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; i--)
    {
        btCollisionObject* obj  = dynamicsWorld->getCollisionObjectArray()[i];
        btRigidBody* rigidBody  = btRigidBody::upcast(obj);
        btCollisionShape* shape = rigidBody->getCollisionShape();
        dynamicsWorld->removeRigidBody(rigidBody);
        dynamicsWorld->removeCollisionObject(obj);

        delete shape;
        delete obj;
    }

    wereColliding.clear();
    areColliding.clear();
    collisionObjects.clear();
    bodiesToRemove.clear();
}

void PhysicsModule::RebuildWorld()
{
    const auto& allGameObjects = App->GetSceneModule()->GetScene()->GetAllGameObjects();

    for (const auto& gameObject : allGameObjects)
    {
        gameObject.second->ParentUpdatedComponents();
    }
}
