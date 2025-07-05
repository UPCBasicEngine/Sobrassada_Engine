#pragma once

#include "ComponentUtils.h"
#include "Module.h"
#include "BulletUserPointer.h"

#include <bitset>
#include <map>
#include <set>
#include <vector>

class btDefaultCollisionConfiguration;
class btCollisionDispatcher;
class btBroadphaseInterface;
class btSequentialImpulseConstraintSolver;
class btDiscreteDynamicsWorld;
class BulletDebugDraw;
class btRigidBody;

class CubeColliderComponent;
class SphereColliderComponent;
class CapsuleColliderComponent;

constexpr float DEFAULT_GRAVITY = -9.81f;

typedef std::bitset<sizeof(ColliderLayerStrings) / sizeof(char*)> LayerBitset;

class SOBRASADA_API_ENGINE PhysicsModule : public Module
{
  public:
    PhysicsModule();
    ~PhysicsModule() = default;

    bool Init() override;
    update_status PreUpdate(float deltaTime) override;
    update_status Render(float deltaTime) override;
    update_status PostUpdate(float deltaTime) override;
    bool ShutDown() override;

    void LoadLayerData(const rapidjson::Value* initialState = nullptr);
    void SaveLayerData(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator);
    void EmptyWorld();
    void RebuildWorld();

    void CreateCubeRigidBody(CubeColliderComponent* colliderComponent);
    void UpdateCubeRigidBody(CubeColliderComponent* colliderComponent);
    void DeleteCubeRigidBody(CubeColliderComponent* colliderComponent);

    void CreateSphereRigidBody(SphereColliderComponent* colliderComponent);
    void UpdateSphereRigidBody(SphereColliderComponent* colliderComponent);
    void DeleteSphereRigidBody(SphereColliderComponent* colliderComponent);

    void CreateCapsuleRigidBody(CapsuleColliderComponent* colliderComponent);
    void UpdateCapsuleRigidBody(CapsuleColliderComponent* colliderComponent);
    void DeleteCapsuleRigidBody(CapsuleColliderComponent* colliderComponent);

    float GetGravity() const { return gravity; }
    std::vector<LayerBitset>& GetLayerConfig() { return colliderLayerConfig; }

    void SetGravity(float newGravity)
    {
        gravity       = newGravity;
        updateGravity = true;
    };

    void SetDebugOption(int option);

  private:
    void AddRigidBody(btRigidBody* rigidBody, ColliderType colliderType, ColliderLayer layerType);

  private:
    float gravity                                           = DEFAULT_GRAVITY;
    bool updateGravity                                      = true;

    btDefaultCollisionConfiguration* collisionConfiguration = nullptr;
    btCollisionDispatcher* dispatcher                       = nullptr;
    btBroadphaseInterface* broadPhase                       = nullptr;
    btSequentialImpulseConstraintSolver* solver             = nullptr;
    btDiscreteDynamicsWorld* dynamicsWorld                  = nullptr;

    std::vector<btRigidBody*> bodiesToRemove;

    BulletDebugDraw* debugDraw = nullptr;
    std::vector<LayerBitset> colliderLayerConfig;

    std::map<UID, std::set<UID>> wereColliding;
    std::map<UID, std::set<UID>> areColliding;
    std::map<UID, BulletUserPointer> collisionObjects;
};