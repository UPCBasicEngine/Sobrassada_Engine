#include "CharacterControllerComponent.h"

#include "Application.h"
#include "CameraComponent.h"
#include "CameraModule.h"
#include "DetourNavMeshQuery.h"
#include "EditorUIModule.h"
#include "GameObject.h"
#include "GameTimer.h"
#include "InputModule.h"
#include "PathfinderModule.h"
#include "ResourceNavMesh.h"
#include "ResourcesModule.h"
#include "SceneModule.h"
#include "Utils/RaycastController.h"

#include "Geometry/LineSegment.h"
#include "Geometry/Plane.h"
#include "Math/Mathfunc.h"
#include "Math/float3.h"
#include "Math/float4x4.h"
#include <SDL_mouse.h>
#include <algorithm>
#include <cmath>

CharacterControllerComponent::CharacterControllerComponent(UID uid, GameObject* parent)
    : Component(uid, parent, "Character Controller", COMPONENT_CHARACTER_CONTROLLER)
{
    maxAngularSpeed = 90 / RAD_DEGREE_CONV;
    isRadians       = true;
    targetDirection.Set(0.0f, 0.0f, 0.0f);
}

CharacterControllerComponent::CharacterControllerComponent(const rapidjson::Value& initialState, GameObject* parent)
    : Component(initialState, parent)
{
    if (initialState.HasMember("TargetDirectionX"))
    {
        targetDirection.x = initialState["TargetDirectionX"].GetFloat();
    }
    if (initialState.HasMember("TargetDirectionY"))
    {
        targetDirection.y = initialState["TargetDirectionY"].GetFloat();
    }
    if (initialState.HasMember("TargetDirectionZ"))
    {
        targetDirection.z = initialState["TargetDirectionZ"].GetFloat();
    }
    if (initialState.HasMember("Speed"))
    {
        maxSpeed = initialState["Speed"].GetFloat();
    }
    if (initialState.HasMember("Acceleration"))
    {
        acceleration = initialState["Acceleration"].GetFloat();
    }
    if (initialState.HasMember("DashDistance"))
    {
        dashDistance = initialState["DashDistance"].GetFloat();
    }
    if (initialState.HasMember("DashDuration"))
    {
        dashDuration = initialState["DashDuration"].GetFloat();
    }
    if (initialState.HasMember("MaxAngularSpeed"))
    {
        maxAngularSpeed = initialState["MaxAngularSpeed"].GetFloat();
    }
    if (initialState.HasMember("isRadians"))
    {
        isRadians = initialState["isRadians"].GetBool();
    }
}

CharacterControllerComponent::~CharacterControllerComponent()
{
}

void CharacterControllerComponent::Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator)
    const
{
    Component::Save(targetState, allocator);

    targetState.AddMember("TargetDirectionX", targetDirection.x, allocator);
    targetState.AddMember("TargetDirectionY", targetDirection.y, allocator);
    targetState.AddMember("TargetDirectionZ", targetDirection.z, allocator);
    targetState.AddMember("Speed", maxSpeed, allocator);
    targetState.AddMember("Acceleration", acceleration, allocator);
    targetState.AddMember("DashDistance", dashDistance, allocator);
    targetState.AddMember("DashDuration", dashDuration, allocator);
    targetState.AddMember("MaxAngularSpeed", maxAngularSpeed, allocator);
    targetState.AddMember("isRadians", isRadians, allocator);
}

void CharacterControllerComponent::Clone(const Component* other)
{
    if (other->GetType() == ComponentType::COMPONENT_CHARACTER_CONTROLLER)
    {
        const CharacterControllerComponent* otherCharacter = static_cast<const CharacterControllerComponent*>(other);
        enabled                                            = otherCharacter->enabled;
        wasEnabled                                         = otherCharacter->wasEnabled;

        maxSpeed                                           = otherCharacter->maxSpeed;
        acceleration                                       = otherCharacter->acceleration;
        dashDuration                                       = otherCharacter->dashDuration;
        dashDistance                                       = otherCharacter->dashDistance;
        maxAngularSpeed                                    = otherCharacter->maxAngularSpeed;

        isRadians                                          = otherCharacter->isRadians;
    }
    else
    {
        GLOG("It is not possible to clone a component of a different type!");
    }
}

void CharacterControllerComponent::Update(float time) // SO many navmesh getters!!!! Memo to rethink this
{
    if (!IsEffectivelyEnabled() || !inputDown) return;

    if (!App->GetSceneModule()->GetInPlayMode()) return;

    float deltaTime = App->GetGameTimer()->GetDeltaTime() / 1000.0f;

    if (deltaTime == 0.0f) return;

    float3 currentPos    = parent->GetGlobalTransform().TranslatePart();
    lastPosition         = currentPos;

    ResourceNavMesh* nav = App->GetPathfinderModule()->GetNavMesh();
    dtNavMesh* dtNav     = nullptr;
    if (nav != nullptr)
    {
        dtNav = nav->GetDetourNavMesh();
    }

    dtNavMeshQuery* tmpQuery = App->GetPathfinderModule()->GetDetourNavMeshQuery();

    if (!tmpQuery || !dtNav) return;

    if (!navMeshQuery)
    {
        navMeshQuery = tmpQuery;

        if (currentPolyRef == 0)
        {
            float3 startPos = parent->GetGlobalTransform().TranslatePart();

            dtQueryFilter filter;
            filter.setIncludeFlags(SAMPLE_POLYFLAGS_WALK);
            filter.setExcludeFlags(0);

            float extents[3] = {2.0f, 4.0f, 2.0f};
            float nearestPoint[3];
            dtPolyRef targetRef = 0;

            dtStatus status = navMeshQuery->findNearestPoly(startPos.ptr(), extents, &filter, &targetRef, nearestPoint);

            if (dtStatusFailed(status) || targetRef == 0)
            {
                //GLOG("Failed to find valid target poly for movement.");
                return;
            }

            currentPolyRef = targetRef;
        }
    }

    if (!navMeshQuery || currentPolyRef == 0) return;

    if (deltaTime < 0.1f) // TODO: deltaTime spikes, need to know why
    {
        verticalSpeed += gravity * deltaTime;
        verticalSpeed  = std::max(verticalSpeed, maxFallSpeed); // Clamp fall speed

        currentPos.y  += (verticalSpeed * deltaTime);

        AdjustHeightToNavMesh(currentPos);
        parent->SetLocalPosition(currentPos - parent->GetParentGlobalTransform().TranslatePart());
    }

    if (!isDashing && isRotating)
    {
        LookAtMovement(rotateDirection, deltaTime);
    }

    if (isDashing) Dash(deltaTime);
    else Move(deltaTime);
}

void CharacterControllerComponent::Render(float deltaTime)
{
    if (!IsEffectivelyEnabled()) return;
}

void CharacterControllerComponent::RenderDebug(float deltaTime)
{
}

void CharacterControllerComponent::RenderEditorInspector()
{
    Component::RenderEditorInspector();

    ImGui::SeparatorText("Character Controller Component");

    float availableWidth = ImGui::GetContentRegionAvail().x;

    ImGui::Separator();
    ImGui::Text("Character Controller");

    ImGui::DragFloat("Max Speed", &maxSpeed, 0.1f, 0.0f, 100.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
    ImGui::DragFloat("Acceleration", &acceleration, 0.1f, 0.0f, 100.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
    ImGui::DragFloat("Dash Distance", &dashDistance, 3.0f, 0.0f, 10.0f, "%.1f", ImGuiSliderFlags_AlwaysClamp);
    ImGui::DragFloat("Dash Duration", &dashDuration, 0.2f, 0.0f, 1.0f, "%.1f", ImGuiSliderFlags_AlwaysClamp);

    float dragStep = isRadians ? 1.0f / RAD_DEGREE_CONV : 1.0f;
    float minVal   = 0.0f;
    float maxVal   = isRadians ? 360.0f / RAD_DEGREE_CONV : 360.0f;

    ImGui::DragFloat(
        "Max Angular Speed##maxAngSpeed", &maxAngularSpeed, dragStep, minVal, maxVal, "%.3f",
        ImGuiSliderFlags_AlwaysClamp
    );

    if (maxAngularSpeed > maxVal) maxAngularSpeed = maxVal;

    bool prevUseRad = isRadians;

    ImGui::SameLine();
    ImGui::Checkbox("Radians##maxAngCheck", &isRadians);

    if (isRadians != prevUseRad)
    {
        if (isRadians)
        {
            maxAngularSpeed /= RAD_DEGREE_CONV;
        }
        else
        {
            maxAngularSpeed *= RAD_DEGREE_CONV;
        }
    }
}

void CharacterControllerComponent::AdjustHeightToNavMesh(float3& currentPos)
{
    if (!navMeshQuery || currentPolyRef == 0) return;

    isGrounded = false;

    dtQueryFilter filter;
    filter.setIncludeFlags(SAMPLE_POLYFLAGS_WALK);
    filter.setExcludeFlags(0);

    float halfExt[3] = {0.5f, 1.0f, 0.5f};
    float nearest[3];
    dtPolyRef newRef = 0;

    dtStatus st      = navMeshQuery->findNearestPoly(currentPos.ptr(), halfExt, &filter, &newRef, nearest);
    if (!dtStatusSucceed(st) || newRef == 0) return;

    bool posOverPoly = false;
    float closest[3];
    dtStatus st2 = navMeshQuery->closestPointOnPoly(newRef, currentPos.ptr(), closest, &posOverPoly);
    if (!dtStatusSucceed(st2) || !posOverPoly) return;

    currentPolyRef   = newRef;

    float polyHeight = 0.0f;
    dtStatus stH     = navMeshQuery->getPolyHeight(newRef, closest, &polyHeight);
    if (dtStatusSucceed(stH))
    {
        isGrounded                = true;
        float distToFloor         = polyHeight - currentPos.y;
        const float maxStepHeight = 0.2f;
        if (distToFloor >= 0.0f && distToFloor <= maxStepHeight)
        {
            currentPos.y  = polyHeight;
            verticalSpeed = 0.0f;
        }
    }
}

void CharacterControllerComponent::Move(float deltaTime)
{
    if (!navMeshQuery || currentPolyRef == 0) return;

    const float3& currentPos = parent->GetGlobalTransform().TranslatePart();
    if (!movementEnabled)
    {
        currentSpeed = 0;
    }
    else
    {
        currentSpeed = targetDirection.LengthSq() > 0.001f ? Lerp(currentSpeed, maxSpeed, acceleration * deltaTime)
                                                           : Lerp(currentSpeed, 0, 100 * deltaTime);
    }

    const float3 offsetXZ = rotateDirection * currentSpeed * deltaTime;
    float3 desiredPos     = currentPos + offsetXZ;

    dtQueryFilter filter;
    filter.setIncludeFlags(SAMPLE_POLYFLAGS_WALK);
    filter.setExcludeFlags(0);

    float halfExt[3] = {1.0f, 1.0f, 1.5f};
    float nearest[3] = {};
    dtPolyRef newRef = 0;

    dtStatus status  = navMeshQuery->findNearestPoly(desiredPos.ptr(), halfExt, &filter, &newRef, nearest);

    if (!dtStatusSucceed(status) || newRef == 0) return;

    float closest[3] = {};
    bool posOverPoly = false;

    status           = navMeshQuery->closestPointOnPoly(newRef, desiredPos.ptr(), closest, &posOverPoly);

    if (!dtStatusSucceed(status)) return;

    currentPolyRef = newRef;

    desiredPos.x   = closest[0];
    desiredPos.y   = closest[1];
    desiredPos.z   = closest[2];

    // Prevent huge changes in the y pos
    if (fabs(desiredPos.y - currentPos.y) > 0.5f) return;

    parent->SetLocalPosition(desiredPos - parent->GetParentGlobalTransform().TranslatePart());
}

void CharacterControllerComponent::LookAtMovement(const float3& moveDir, float deltaTime)
{
    if (moveDir.LengthSq() < 0.0001f) return;

    float3 desiredDir = moveDir;
    desiredDir.y      = 0.0f;
    desiredDir.Normalize();

    const float4x4& localTransform = parent->GetLocalTransform();
    float3 forward                 = localTransform.WorldZ();
    forward.y                      = 0.0f;
    forward.Normalize();

    float angle   = atan2(forward.Cross(desiredDir).y, forward.Dot(desiredDir));

    float maxStep = maxAngularSpeed * deltaTime;
    if (isRadians) maxStep *= RAD_DEGREE_CONV;
    angle = std::clamp(angle, -maxStep, maxStep);

    if (fabs(angle) < 0.00001f)
    {
        isRotating = false;
        return;
    }

    const float4x4 rotated = localTransform * float4x4::FromEulerXYZ(0.0f, angle, 0.0f);
    parent->SetLocalTransform(rotated);
}

void CharacterControllerComponent::Rotate(float rotationDirection, float deltaTime)
{
    float angleDeg = 0.0f;

    if (isRadians)
    {
        angleDeg = maxAngularSpeed * rotationDirection * deltaTime;
    }
    else
    {
        angleDeg = (maxAngularSpeed * rotationDirection * deltaTime) / RAD_DEGREE_CONV;
    }

    float4x4 rotationMatrix = float4x4::FromEulerXYZ(0.0f, angleDeg, 0.0f);

    float4x4 localTr        = parent->GetLocalTransform();

    localTr                 = localTr * rotationMatrix;

    parent->SetLocalTransform(localTr);
    parent->UpdateTransformForGOBranch();
}

void CharacterControllerComponent::SetDirection(float3& direction)
{
    if (!movementEnabled) return;

    targetDirection = direction;
    if (direction.LengthSq() > 0.001f)
    {
        direction.Normalize();
        targetDirection = direction;
        rotateDirection = direction;
        isRotating      = true;
    }
}

void CharacterControllerComponent::LookAt(const float3& direction)
{
    isRotating      = true;
    rotateDirection = direction;

    // Set 1 as deltaTime so it rotates immediatly
    LookAtMovement(rotateDirection, 1.0f);
}

float2 CharacterControllerComponent::GetRealSpeed() const
{
    const float deltaTime       = App->GetGameTimer()->GetDeltaTime() / 1000.0f;

    const float3 positionsDiff  = parent->GetGlobalTransform().TranslatePart() - lastPosition;
    const float horizontalSpeed = float2(positionsDiff.x / deltaTime, positionsDiff.z / deltaTime).Length();
    const float verticalSpeed   = positionsDiff.y / deltaTime;

    return {horizontalSpeed, verticalSpeed};

    // horizontalSpeed.Length() / deltaTime;
}

void CharacterControllerComponent::StartDash()
{
    isDashing         = true;

    // WALL COLLISION LOGIC
    float3 currentPos = parent->GetGlobalTransform().TranslatePart();
    dashTarget        = currentPos + rotateDirection * (dashDistance + 0.5f);
    dashSpeed         = dashDistance / dashDuration;
    dashTimeRemaining = dashDuration;

    // If the dash ignores the navMesh, the code below is not needed

    // const float3 lateralDirection  = rotateDirection.Cross(float3::unitY).Normalized();
    //
    // currentPos.y                  += 0.5f;
    // float3 rightRayOrigin          = currentPos + lateralDirection * 0.5f;
    // float3 leftRayOrigin           = currentPos - lateralDirection * 0.5f;
    //
    // LineSegment centralRay(currentPos, dashTarget);
    // LineSegment rightRay(rightRayOrigin, rightRayOrigin + rotateDirection * (dashDistance + 0.5f));
    // LineSegment leftRay(leftRayOrigin, leftRayOrigin + rotateDirection * (dashDistance + 0.5f));
    //
    // GameObject* centralHit = RaycastController::GetRayIntersectionTrees(
    //     centralRay, App->GetSceneModule()->GetScene()->GetOctree(),
    //     App->GetSceneModule()->GetScene()->GetDynamicTree()
    //);
    // GameObject* rightHit = RaycastController::GetRayIntersectionTrees(
    //     rightRay, App->GetSceneModule()->GetScene()->GetOctree(), App->GetSceneModule()->GetScene()->GetDynamicTree()
    //);
    // GameObject* leftHit = RaycastController::GetRayIntersectionTrees(
    //     leftRay, App->GetSceneModule()->GetScene()->GetOctree(), App->GetSceneModule()->GetScene()->GetDynamicTree()
    //);
    //
    // const float wallOffset = 0.7f;
    // float tNear, tFar;
    //
    // if (centralHit != nullptr)
    //{
    //     const AABB& box = centralHit->GetGlobalAABB();
    //     if (box.Intersects(centralRay, tNear, tFar))
    //     {
    //         dashTarget = centralRay.GetPoint(tNear) - rotateDirection * wallOffset;
    //     }
    // }
    // else if (rightHit != nullptr)
    //{
    //     const AABB& box = rightHit->GetGlobalAABB();
    //     if (box.Intersects(rightRay, tNear, tFar))
    //     {
    //         dashTarget = (rightRay.GetPoint(tNear) - rotateDirection * wallOffset) - lateralDirection * 0.5f;
    //     }
    // }
    // else if (leftHit != nullptr)
    //{
    //     const AABB& box = leftHit->GetGlobalAABB();
    //     if (box.Intersects(leftRay, tNear, tFar))
    //     {
    //         dashTarget = (leftRay.GetPoint(tNear) - rotateDirection * wallOffset) + lateralDirection * 0.5f;
    //     }
    // }

    //// NOT FALLING LOGIC

    // dtQueryFilter filter;
    // filter.setIncludeFlags(SAMPLE_POLYFLAGS_WALK);
    // filter.setExcludeFlags(0);

    // float extents[3]      = {0.1f, 1.0f, 0.1f}; // Tama�o de la caja de b�squeda
    // float nearestPoint[3] = {0.0f, 0.0f, 0.0f};
    // dtPolyRef targetRef   = 0;

    // dtStatus status       = navMeshQuery->findNearestPoly(dashTarget.ptr(), extents, &filter, &targetRef,
    // nearestPoint);

    // if (dtStatusFailed(status) || targetRef == 0)
    //{
    //     GLOG("Nearest points: (%f, %f, %f)", nearestPoint[0], nearestPoint[1], nearestPoint[2]);
    //     GLOG("No navmesh found at dash target position. Dash canceled.");
    //     dashTarget = float3(nearestPoint[0], nearestPoint[1], nearestPoint[2]);
    // }
}

void CharacterControllerComponent::Dash(float deltaTime)
{
    if (!navMeshQuery || currentPolyRef == 0) return;

    if (dashTimeRemaining <= 0.0f)
    {
        isDashing = false;
    }

    const float3 currentPos  = parent->GetGlobalTransform().TranslatePart();
    float3 directionToTarget = dashTarget - currentPos;
    float distanceToTarget   = directionToTarget.Length();

    if (distanceToTarget > 0.1f)
    {
        directionToTarget.Normalize();
        const float3 dashOffset = directionToTarget * dashSpeed * deltaTime;
        float3 desiredPos       = currentPos + dashOffset;

        dtQueryFilter filter;
        filter.setIncludeFlags(SAMPLE_POLYFLAGS_WALK);
        filter.setExcludeFlags(0);

        float halfExt[3] = {1.0f, 1.0f, 1.5f};
        float nearest[3] = {};
        dtPolyRef newRef = 0;

        dtStatus status  = navMeshQuery->findNearestPoly(desiredPos.ptr(), halfExt, &filter, &newRef, nearest);

        if (!dtStatusSucceed(status) || newRef == 0) return;

        float closest[3] = {};
        bool posOverPoly = false;

        status           = navMeshQuery->closestPointOnPoly(newRef, desiredPos.ptr(), closest, &posOverPoly);

        if (!dtStatusSucceed(status)) return;

        currentPolyRef = newRef;

        desiredPos.x   = closest[0];
        desiredPos.y   = closest[1];
        desiredPos.z   = closest[2];

        // Prevent huge changes in the y pos
        if (fabs(desiredPos.y - currentPos.y) > 0.5f) return;

        if (dashOffset.Length() >= distanceToTarget)
        {
            parent->SetLocalPosition(dashTarget - parent->GetParentGlobalTransform().TranslatePart());
            dashTimeRemaining = 0.0f;
            isDashing         = false;
        }
        else
        {
            parent->SetLocalPosition(desiredPos - parent->GetParentGlobalTransform().TranslatePart());
            dashTimeRemaining -= deltaTime;
        }
    }
    else
    {
        isDashing         = false;
        dashTimeRemaining = 0.0f;
    }
}