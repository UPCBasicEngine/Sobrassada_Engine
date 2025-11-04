#include "CharacterControllerComponent.h"

#include "Application.h"
#include "BulletUserPointer.h"
#include "CameraComponent.h"
#include "CameraModule.h"
#include "DebugDrawModule.h"
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
    if (initialState.HasMember("PreciseDash"))
    {
        isRadians = initialState["PreciseDash"].GetBool();
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
    targetState.AddMember("PreciseDash", preciseDash, allocator);
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

void CharacterControllerComponent::Init()
{

    float3 startPos  = parent->GetGlobalTransform().TranslatePart();
    lastPosition     = startPos;
    previousPosition = startPos;
}

void CharacterControllerComponent::Update(float time) // SO many navmesh getters!!!! Memo to rethink this
{
    if (!IsEffectivelyEnabled() || !inputDown) return;
    if (!App->GetSceneModule()->GetInPlayMode()) return;

    float deltaTime = App->GetGameTimer()->GetDeltaTime() / 1000.0f;
    if (deltaTime == 0.0f) return;

    float3 currentPos = parent->GetGlobalTransform().TranslatePart();

    if (deltaTime > 0.0f && deltaTime < 0.1f)
    {
        float3 frameVelocity                 = (currentPos - previousPosition) / deltaTime;

        velocitySamples[velocitySampleIndex] = frameVelocity;
        velocitySampleIndex                  = (velocitySampleIndex + 1) % VELOCITY_SAMPLES;

        float3 velocitySum                   = float3::zero;
        for (int i = 0; i < VELOCITY_SAMPLES; i++)
        {
            velocitySum += velocitySamples[i];
        }
        currentVelocity = velocitySum / VELOCITY_SAMPLES;
    }

    previousPosition     = currentPos;
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
        navMeshQuery    = tmpQuery;

        float3 startPos = parent->GetGlobalTransform().TranslatePart();

        dtQueryFilter filter;
        filter.setIncludeFlags(SAMPLE_POLYFLAGS_WALK);
        filter.setExcludeFlags(0);

        float extents[3] = {10.0f, 10.0f, 10.0f};
        float nearestPoint[3];
        dtPolyRef targetRef = 0;

        dtStatus status     = navMeshQuery->findNearestPoly(startPos.ptr(), extents, &filter, &targetRef, nearestPoint);

        if (dtStatusFailed(status) || targetRef == 0)
        {
            return;
        }
    }

    if (deltaTime < 0.1f && !isDashing) // Your existing deltaTime check
    {
        verticalSpeed += gravity * deltaTime;
        verticalSpeed  = std::max(verticalSpeed, maxFallSpeed);

        currentPos.y  += std::max(-0.5f, verticalSpeed * deltaTime);

        AdjustHeightToNavMesh(currentPos, deltaTime);
    }

    if (!isDashing && isRotating)
    {
        LookAtMovement(rotateDirection, deltaTime);
    }

    if (isDashing) Dash(deltaTime);
    else Move(deltaTime);
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
    ImGui::DragFloat("Walk Speed", &walkSpeed, 0.1f, 0.0f, 100.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
    ImGui::DragFloat("Acceleration", &acceleration, 0.1f, 0.0f, 100.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
    ImGui::DragFloat("Dash Distance", &dashDistance, 3.0f, 0.0f, 10.0f, "%.1f", ImGuiSliderFlags_AlwaysClamp);
    ImGui::DragFloat("Dash Duration", &dashDuration, 0.2f, 0.0f, 1.0f, "%.1f", ImGuiSliderFlags_AlwaysClamp);
    ImGui::Checkbox("Precise Dash", &preciseDash);

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

void CharacterControllerComponent::AdjustHeightToNavMesh(float3 currentPos, float deltaTime)
{
    if (!navMeshQuery) return;

    dtQueryFilter filter;
    filter.setIncludeFlags(SAMPLE_POLYFLAGS_WALK);
    filter.setExcludeFlags(0);

    // These numbers are the distance considering 60 fps as default -> 12.5 * 0.16ms = 0.2f & 1875 * 0.16ms = 30.0
    // search area. If lower framerate, the are is bigger because the steps each frame are longer. If higher framerate,
    // the are is smaller because the steps between frames are way smoother
    float halfExt[3] = {
        12.5f * deltaTime, std::max(1875.0f * deltaTime, 30.0f), 12.5f * deltaTime
    }; // Big Y search because we want to check for navmeshes below to fall
    float nearest[3];
    dtPolyRef newRef = 0;

    dtStatus st      = navMeshQuery->findNearestPoly(currentPos.ptr(), halfExt, &filter, &newRef, nearest);
    if (!dtStatusSucceed(st) || newRef == 0) return;

    bool posOverPoly = false;
    float closest[3];
    dtStatus st2 = navMeshQuery->closestPointOnPoly(newRef, currentPos.ptr(), closest, &posOverPoly);
    if (!dtStatusSucceed(st2) || !posOverPoly) return;

    float polyHeight = 0.0f;
    dtStatus stH     = navMeshQuery->getPolyHeight(newRef, closest, &polyHeight);
    if (dtStatusSucceed(stH))
    {
        float distToFloor         = abs(polyHeight - currentPos.y);
        const float maxStepHeight = 0.5f;
        if (distToFloor > 0.0f && distToFloor <= maxStepHeight)
        {
            isGrounded    = true;
            currentPos.y  = polyHeight;
            verticalSpeed = 0.0f;
        }
        else
        {
            isGrounded = false;
        }

        parent->SetLocalPosition(currentPos - parent->GetParentGlobalTransform().TranslatePart());
    }
}

void CharacterControllerComponent::Move(float deltaTime)
{
    if (!navMeshQuery) return;

    const float3& currentPos = parent->GetGlobalTransform().TranslatePart();
    if (!movementEnabled)
    {
        currentSpeed = 0;
    }
    else
    {
        const float desiredSpeed = isRunning ? maxSpeed : walkSpeed;
        currentSpeed             = targetDirection.LengthSq() > 0.001f
                                     ? Lerp(currentSpeed, desiredSpeed, std::min(1.f, acceleration * deltaTime))
                                     : Lerp(currentSpeed, 0, std::min(1.f, 100 * deltaTime));
    }

    const float3 offsetXZ   = rotateDirection * currentSpeed * deltaTime;
    const float3 desiredPos = currentPos + offsetXZ;

    // These numbers are the distance considering 60 fps as default -> 25.0 * 0.16ms = 0.4f search area.
    // If lower framerate, the are is bigger because the steps each frame are longer.
    // If higher framerate, the are is smaller because the steps between frames are way smoother
    const float3 searchArea = {25.0f * deltaTime, 25.0f * deltaTime, 25.0f * deltaTime};
    float3 closestPoint     = float3::zero;
    bool posOverPoly        = false;
    dtStatus status         = GetClosestPointInNavmesh(desiredPos, searchArea, posOverPoly, closestPoint);

    if (!dtStatusSucceed(status)) return;

    // Prevent huge changes
    if (fabs(closestPoint.x - currentPos.x) > 12.5f * deltaTime ||
        fabs(closestPoint.y - currentPos.y) > 25.0f * deltaTime ||
        fabs(closestPoint.z - currentPos.z) > 12.5f * deltaTime)
        return;

    parent->SetLocalPosition(closestPoint - parent->GetParentGlobalTransform().TranslatePart());
}

void CharacterControllerComponent::MoveTo(float speed)
{
    float deltaTime          = App->GetGameTimer()->GetDeltaTime() / 1000.0f;
    const float3& currentPos = parent->GetGlobalTransform().TranslatePart();
    const float3 offsetXZ    = rotateDirection * speed * deltaTime;
    const float3 desiredPos  = currentPos + offsetXZ;

    const float3 searchArea  = {1.0f, 1.0f, 1.0f};
    float3 closestPoint      = float3::zero;
    bool posOverPoly         = false;
    dtStatus status          = GetClosestPointInNavmesh(desiredPos, searchArea, posOverPoly, closestPoint);

    if (!dtStatusSucceed(status)) return;

    // Prevent huge changes in the y pos
    if (fabs(closestPoint.y - currentPos.y) > 0.5f) return;

    parent->SetLocalPosition(closestPoint - parent->GetParentGlobalTransform().TranslatePart());
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

void CharacterControllerComponent::SetDirection(const float3& direction)
{
    if (!movementEnabled) return;

    targetDirection = direction;
    if (direction.LengthSq() > 0.001f)
    {
        float3 dir = direction;
        dir.Normalize();
        targetDirection = dir;
        rotateDirection = dir;
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
    isDashing               = true;

    const float3 currentPos = parent->GetGlobalTransform().TranslatePart();
    const float3 finalPos   = currentPos + rotateDirection * dashDistance;
    dashDirection           = rotateDirection;
    dashSpeed               = dashDistance / dashDuration;
    dashTimeRemaining       = dashDuration;

    const float3 searchArea = {0.2f, 30.0f, 0.2f};
    float3 closestPoint     = float3::zero;
    bool posOverPoly        = false;
    dtStatus status         = GetClosestPointInNavmesh(finalPos, searchArea, posOverPoly, closestPoint);
    dashToNavmesh           = posOverPoly && closestPoint.y <= finalPos.y + 0.2f;
    // GLOG("Dash to navmesh? %d", dashToNavmesh);

    if (!dashToNavmesh) return;

    // If it's dashing to a point in the navmesh, check there are no obstacles in the path
    CheckDashObstacles();
}

void CharacterControllerComponent::Dash(float deltaTime)
{
    dashMoveBlocked = false;

    CheckDashObstacles();

    const float animStep = std::min(deltaTime, dashTimeRemaining);
    const float moveStep = dashMoveBlocked ? 0.0f : animStep;

    if (moveStep == 0.0f)
    {
        dashTimeRemaining -= animStep;
        if (dashTimeRemaining <= 0.0f)
        {
            isDashing       = false;
            dashMoveBlocked = false;
        }
        return;
    }

    const float3 currentPos = parent->GetGlobalTransform().TranslatePart();

    const float3 dashOffset = dashDirection * dashSpeed * moveStep;
    float3 desiredPos       = currentPos + dashOffset;

    const float3 searchArea = {62.5f * moveStep, std::max(0.4f, 25.0f * moveStep), 62.5f * moveStep};
    bool posOverPoly        = false;
    float3 closestPoint     = float3::zero;

    dtStatus status         = GetClosestPointInNavmesh(desiredPos, searchArea, posOverPoly, closestPoint);

    if (!dashToNavmesh || obstacleInDash || (posOverPoly && dashToNavmesh)) desiredPos = closestPoint;

    parent->SetLocalPosition(desiredPos - parent->GetParentGlobalTransform().TranslatePart());
    dashTimeRemaining -= animStep;

    if (dashTimeRemaining > 0.05f && preciseDash)
    {
        // Check if the end of the remaining dash is inside the navmesh in case we are sliding next to the edge
        const float3 currentPos2 = parent->GetGlobalTransform().TranslatePart();
        const float3 finalPos    = currentPos2 + dashDirection * dashSpeed * dashTimeRemaining;

        const float3 searchArea2 = {12.5f * moveStep, std::max(1875.0f * moveStep, 30.0f), 12.5f * moveStep};
        float3 closestPoint2     = float3::zero;
        bool posOverPoly2        = false;
        dtStatus status          = GetClosestPointInNavmesh(finalPos, searchArea2, posOverPoly2, closestPoint2);
        dashToNavmesh            = posOverPoly2 && closestPoint2.y <= finalPos.y + 0.2f;

        if (dashToNavmesh) CheckDashObstacles();
    }

    if (dashTimeRemaining <= 0.0f)
    {
        isDashing       = false;
        dashMoveBlocked = false;
    }
}

void CharacterControllerComponent::CheckDashObstacles()
{
    obstacleInDash                 = false;

    const float3 lateralDirection  = dashDirection.Cross(float3::unitY).Normalized();
    float3 currentPos              = parent->GetGlobalTransform().TranslatePart();
    currentPos.y                  += 0.75f;
    float3 rightRayOrigin          = currentPos + lateralDirection * 0.2f;
    float3 leftRayOrigin           = currentPos - lateralDirection * 0.2f;

    LineSegment centralRay(currentPos + dashDirection * 0.1f, currentPos + dashDirection * 3.0f);
    LineSegment rightRay(rightRayOrigin + dashDirection * 0.15f, rightRayOrigin + dashDirection * 3.0f);
    LineSegment leftRay(leftRayOrigin + dashDirection * 0.15f, leftRayOrigin + dashDirection * 3.0f);

    GameObject* centralHit =
        RaycastController::GetRayIntersectionTrees(centralRay, App->GetSceneModule()->GetScene()->GetOctree());
    GameObject* rightHit =
        RaycastController::GetRayIntersectionTrees(rightRay, App->GetSceneModule()->GetScene()->GetOctree());
    GameObject* leftHit =
        RaycastController::GetRayIntersectionTrees(leftRay, App->GetSceneModule()->GetScene()->GetOctree());

    GameObject* hitParent          = nullptr;
    BulletUserPointer* userPointer = RaycastController::GetRayIntersectionPhysics(centralRay);
    if (userPointer)
    {
        hitParent = userPointer->collider->GetParent();
        // GLOG("[CharacterControllerComponent]: Physics Raycast hit!, %s", hitParent->GetName().c_str());
    }

    DebugDrawModule* debug = App->GetDebugDrawModule();
    if (debug->GetDebugOptionValue((int)DebugOptions::RENDER_DEBUG_VISUALS))
    {
        float3 centralColor = centralHit != nullptr ? float3(1.0f, 0.0f, 0.0f) : float3(1.0f, 1.0f, 0.0f);
        debug->DrawLineSegment(centralRay, centralColor);

        float3 rightColor = rightHit != nullptr ? float3(1.0f, 0.0f, 0.0f) : float3(1.0f, 1.0f, 0.0f);
        debug->DrawLineSegment(rightRay, rightColor);

        float3 leftColor = leftHit != nullptr ? float3(1.0f, 0.0f, 0.0f) : float3(1.0f, 1.0f, 0.0f);
        debug->DrawLineSegment(leftRay, leftColor);
    }

    const float wallOffset = 0.7f;
    float tNear, tFar;

    // Check GO with blocking tags
    auto hasDashBlockingTag = [](GameObject* go) -> bool
    {
        if (!go) return false;
        for (const char* tagName : DashBlockerGOTags)
        {
            if (go->HasTag(HashString(tagName))) return true;
        }
        return false;
    };

    auto findBlockingAncestor = [&](GameObject* go) -> GameObject*
    {
        GameObject* ancestor = nullptr;
        for (GameObject* p = go; p; p = App->GetSceneModule()->GetScene()->GetGameObjectByUID(p->GetParent()))
            if (hasDashBlockingTag(p)) ancestor = p;
        return ancestor;
    };

    GameObject* barrierGO = nullptr;
    if (!barrierGO) barrierGO = findBlockingAncestor(hitParent);
    if (!barrierGO) barrierGO = findBlockingAncestor(centralHit);
    if (!barrierGO) barrierGO = findBlockingAncestor(rightHit);
    if (!barrierGO) barrierGO = findBlockingAncestor(leftHit);

    float3 basePos        = parent->GetGlobalTransform().TranslatePart();
    basePos.y            += 0.5f;
    const float skin      = 0.12f;
    const float extra     = 0.20f;
    const float clampLen  = std::max(3.0f, dashSpeed * dashTimeRemaining) + extra;

    LineSegment clampRay(basePos, basePos + dashDirection * clampLen);
    BulletUserPointer* userPointerClamp = RaycastController::GetRayIntersectionPhysics(clampRay);
    GameObject* hitParentClamp          = userPointerClamp ? userPointerClamp->collider->GetParent() : nullptr;

    if (!barrierGO) barrierGO = findBlockingAncestor(hitParentClamp);

    if (barrierGO)
    {
        const AABB& box = barrierGO->GetGlobalAABB();

        if (box.Contains(basePos))
        {
            dashMoveBlocked = true;
            obstacleInDash  = true;
            return;
        }

        if (box.Intersects(clampRay, tNear, tFar))
        {
            const float3 hitPos = clampRay.GetPoint(tNear);

            const float skin    = 0.12f;
            float forward       = (hitPos - basePos).Dot(dashDirection);
            float stopDist      = std::max(0.0f, forward - skin);

            if (stopDist <= skin)
            {
                dashMoveBlocked = true;
                obstacleInDash  = true;
                return;
            }

            const float timeToStop = (dashSpeed > 0.0f) ? (stopDist / dashSpeed) : 0.0f;
            dashTimeRemaining      = std::min(dashTimeRemaining, timeToStop);

            dashMoveBlocked        = false;
            obstacleInDash         = true;
            return;
        }
    }
    else
    {
        if (centralHit)
        {
            //GLOG("HIT IN CENTRAL WITH: %s", centralHit->GetName().c_str());
            const AABB& box = centralHit->GetGlobalAABB();
            if (box.Intersects(centralRay, tNear, tFar))
            {
                // Check whether the obstacle is a walkable area
                const float3 searchArea = {0.001f, 0.001f, 0.001f};
                float3 closestPoint     = float3::zero;
                const float* pos        = centralRay.GetPoint(tNear).ptr();
                const float3 searchPos  = {pos[0], pos[1], pos[2]};
                bool posOverPoly        = false;

                dtStatus status         = GetClosestPointInNavmesh(searchPos, searchArea, posOverPoly, closestPoint);
                obstacleInDash          = !posOverPoly;
                // GLOG("Hit with central. Dash to navmesh? %d", dashToNavmesh);
                return;
            }
        }

        if (rightHit)
        {
            //GLOG("HIT IN RIGHT WITH: %s", rightHit->GetName().c_str());
            const AABB& box = rightHit->GetGlobalAABB();
            if (box.Intersects(rightRay, tNear, tFar))
            {
                // Check whether the obstacle is a walkable area
                const float3 searchArea = {0.001f, 0.001f, 0.001f};
                float3 closestPoint     = float3::zero;
                const float* pos        = rightRay.GetPoint(tNear).ptr();
                const float3 searchPos  = {pos[0], pos[1], pos[2]};
                bool posOverPoly        = false;

                dtStatus status         = GetClosestPointInNavmesh(searchPos, searchArea, posOverPoly, closestPoint);
                obstacleInDash          = !posOverPoly;
                // GLOG("Hit with right. Dash to navmesh? %d", dashToNavmesh);
                parent->SetLocalPosition(parent->GetPosition() - lateralDirection * 0.2f);
            }
        }

        if (leftHit)
        {
            //GLOG("HIT IN LEFT WITH: %s", leftHit->GetName().c_str());
            const AABB& box = leftHit->GetGlobalAABB();
            if (box.Intersects(leftRay, tNear, tFar))
            {
                // Check whether the obstacle is a walkable area
                const float3 searchArea = {0.001f, 0.001f, 0.001f};
                float3 closestPoint     = float3::zero;
                const float* pos        = leftRay.GetPoint(tNear).ptr();
                const float3 searchPos  = {pos[0], pos[1], pos[2]};
                bool posOverPoly        = false;

                dtStatus status         = GetClosestPointInNavmesh(searchPos, searchArea, posOverPoly, closestPoint);
                obstacleInDash          = !posOverPoly;
                // GLOG("Hit with left. Dash to navmesh? %d", dashToNavmesh);
                parent->SetLocalPosition(parent->GetPosition() + lateralDirection * 0.2f);
            }
        }
    }
}

unsigned int CharacterControllerComponent::GetClosestPointInNavmesh(
    const float3& searchPos, const float3& searchArea, bool& posOverPoly, float3& closestPoint
) const
{
    dtQueryFilter filter;
    filter.setIncludeFlags(SAMPLE_POLYFLAGS_WALK);
    filter.setExcludeFlags(0);
    float halfExt[3] = {searchArea.x, searchArea.y, searchArea.z};
    float nearest[3] = {};
    dtPolyRef newRef = 0;

    dtStatus status  = navMeshQuery->findNearestPoly(searchPos.ptr(), halfExt, &filter, &newRef, nearest);

    if (!dtStatusSucceed(status) || newRef == 0) return status; // If unexpected crash, maybe this is needed

    float closest[3] = {};
    status           = navMeshQuery->closestPointOnPoly(newRef, searchPos.ptr(), closest, &posOverPoly);
    closestPoint     = {closest[0], closest[1], closest[2]};

    return status;
}