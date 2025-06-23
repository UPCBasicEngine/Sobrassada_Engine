#pragma once

#include "BulletMotionState.h"
#include "BulletUserPointer.h"
#include "Component.h"
#include "Delegate.h"

#include "Math/float3.h"
#include <bitset>

class btRigidBody;
class GameObject;

class CubeColliderComponent : public Component
{
  public:
    CubeColliderComponent(UID uid, GameObject* parent);
    CubeColliderComponent(const rapidjson::Value& initialState, GameObject* parent);
    ~CubeColliderComponent() override;

    void Init() override;
    void Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const override;
    void Clone(const Component* other) override;

    void RenderEditorInspector() override;

    void Update(float deltaTime) override;
    void Render(float deltaTime) override;
    void RenderDebug(float deltaTime) override;

    void ParentUpdated() override;

    void SOBRASADA_API_ENGINE OnCollision(GameObject* otherObject, float3 collisionNormal, ColliderLayer layer);
    void SOBRASADA_API_ENGINE OnCollisionEnter(GameObject* otherObject, float3 collisionNormal, ColliderLayer layer);
    void SOBRASADA_API_ENGINE OnCollisionExit(GameObject* otherObject, ColliderLayer layer);

    void SOBRASADA_API_ENGINE DeleteRigidBody();

  private:
    void CalculateCollider();

  public:
    bool generateCallback         = true;
    bool fitToSize                = false;
    float mass                    = 1.f;
    float3 centerOffset           = float3::zero;
    float3 centerRotation         = float3::zero;
    float3 size                   = float3::one;
    ColliderType colliderType     = ColliderType::DYNAMIC;

    btRigidBody* rigidBody        = nullptr;
    BulletMotionState motionState = BulletMotionState(nullptr, float3::zero, float3::zero);

    CollisionDelegate onCollissionCallback;
    CollisionDelegate onCollissionEnterCallback;
    CollisionExitDelegate onCollissionExitCallback;

    ColliderLayer layer           = ColliderLayer::WORLD_OBJECTS;

    BulletUserPointer userPointer = BulletUserPointer(
        this, &onCollissionCallback, &onCollissionEnterCallback, &onCollissionExitCallback, generateCallback, layer
    );
};
