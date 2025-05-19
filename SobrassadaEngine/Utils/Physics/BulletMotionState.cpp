#include "BulletMotionState.h"

#include "Application.h"
#include "Component.h"
#include "GameObject.h"
#include "SceneModule.h"
#include "Standalone/Physics/CubeColliderComponent.h"

#include "Math/Quat.h"
#include "Math/float3.h"
#include "Math/float4x4.h"

BulletMotionState::BulletMotionState(
    Component* newCollider, const math::float3& newCenterOffset, const math::float3& newCenterRotation
)
    : collider(newCollider)
{
    btQuaternion rotationQuat =
        btQuaternion(btScalar(newCenterRotation.y), btScalar(newCenterRotation.x), btScalar(newCenterRotation.z));

    centerOffset = btTransform(
                       rotationQuat,
                       btVector3(btScalar(newCenterOffset.x), btScalar(newCenterOffset.y), btScalar(newCenterOffset.z))
    )
                       .inverse();
}

// Syncronize from render world to physics world
void BulletMotionState::getWorldTransform(btTransform& outPhysicsWorldTransform) const
{
    GameObject* parent              = collider->GetParent();

    const float4x4& globalTransform = parent->GetGlobalTransform();

    float3 globalRotation           = globalTransform.RotatePart().ToEulerXYZ();
    float3 globalPosition           = globalTransform.TranslatePart();

    btTransform gameObjectWorldTransform;

    float3x3 rotationMat = float3x3::FromEulerXYZ(globalRotation.x, globalRotation.y, globalRotation.z);
    btMatrix3x3 bulletRotation;

    for (int i = 0; i < 3; ++i)
    {
        for (int j = 0; j < 3; ++j)
        {
            bulletRotation[i][j] = rotationMat[i][j];
        }
    }

    gameObjectWorldTransform.setBasis(bulletRotation);
    gameObjectWorldTransform.setOrigin(
        btVector3(btScalar(globalPosition.x), btScalar(globalPosition.y), btScalar(globalPosition.z))
    );

    outPhysicsWorldTransform = gameObjectWorldTransform * centerOffset.inverse();
}

// Syncronize from physics world to render world
void BulletMotionState::setWorldTransform(const btTransform& physicsWorldTransform)
{
    if (!App->GetSceneModule()->GetInPlayMode()) return;

    // Physics world transform
    btTransform finalPhysicsTransform;
    finalPhysicsTransform.setIdentity();

    finalPhysicsTransform.setOrigin(physicsWorldTransform.getOrigin());

    // Render world physics collider transform
    btTransform finalOffsetTransform;
    finalOffsetTransform.setIdentity();

    finalOffsetTransform.setOrigin(centerOffset.getOrigin());

    btTransform gameObjectTransform = finalPhysicsTransform * finalOffsetTransform;
    btMatrix3x3 rotationBasis       = gameObjectTransform.getBasis();

    float3x3 rotationMat;
    for (int i = 0; i < 3; ++i)
    {
        for (int j = 0; j < 3; ++j)
        {
            rotationMat[i][j] = rotationBasis[i][j];
        }
    }

    float3 newPosition = float3(
        gameObjectTransform.getOrigin().x(), gameObjectTransform.getOrigin().y(), gameObjectTransform.getOrigin().z()
    );

    float4x4 newLocalMatrix = float4x4::FromTRS(newPosition, rotationMat, collider->GetParent()->GetScale());

    collider->GetParent()->SetLocalTransform(newLocalMatrix);
    collider->GetParent()->SetPosition(newPosition);
    collider->GetParent()->UpdateTransformForGOBranch();
}
