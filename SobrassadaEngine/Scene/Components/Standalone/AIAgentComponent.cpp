#include "AIAgentComponent.h"

#include "Application.h"
#include "DetourNavMeshQuery.h"
#include "EditorUIModule.h"
#include "EngineTimer.h"
#include "GameObject.h"
#include "GameTimer.h"
#include "PathfinderModule.h"
#include "ResourceNavmesh.h"
#include "SceneModule.h"
#include "Standalone/CharacterControllerComponent.h"

#include "DetourCrowd.h"

AIAgentComponent::AIAgentComponent(UID uid, GameObject* parent) : Component(uid, parent, "AI Agent", COMPONENT_AIAGENT)
{
    defaultSpeed        = 3.5f;
    currentSpeed        = defaultSpeed;
    defaultAcceleration = 8.0f;
    currentAcceleration = defaultAcceleration;
    radius              = 0.6f;
    height              = 2.0f;
    maxAngularSpeed     = 360 / RAD_DEGREE_CONV;
    currentAngularSpeed = maxAngularSpeed;
    isRadians           = true;

    RecreateAgent();
}

AIAgentComponent::AIAgentComponent(const rapidjson::Value& initialState, GameObject* parent)
    : Component(initialState, parent)
{
    if (initialState.HasMember("Speed")) defaultSpeed = initialState["Speed"].GetFloat();
    if (initialState.HasMember("Acceleration")) defaultAcceleration = initialState["Acceleration"].GetFloat();
    if (initialState.HasMember("Radius")) radius = initialState["Radius"].GetFloat();
    if (initialState.HasMember("Height")) height = initialState["Height"].GetFloat();
    if (initialState.HasMember("MaxAngularSpeed"))
    {
        maxAngularSpeed = initialState["MaxAngularSpeed"].GetFloat();
    }
    if (initialState.HasMember("isRadians"))
    {
        isRadians = initialState["isRadians"].GetBool();
    }

    RecreateAgent();
}

AIAgentComponent::~AIAgentComponent()
{
    if (agentId != -1)
    {
        App->GetPathfinderModule()->RemoveAgent(agentId);
        agentId = -1;
    }
}

// Updates agent position evey frame
void AIAgentComponent::Update(float deltaTime)
{
    if (!IsEffectivelyEnabled())
    {
        if (agentId != -1)
        {
            App->GetPathfinderModule()->RemoveAgent(agentId);
            agentId = -1;
        }
        return;
    }
    else
    {
        if (agentId == -1) RecreateAgent();
    }

    if (!App->GetSceneModule()->GetInPlayMode()) return;

    dtCrowd* crowd = App->GetPathfinderModule()->GetCrowd();

    if (!crowd) return;

    if (agentId == -1 || agentId >= crowd->getAgentCount() || crowd->getAgent(agentId) == nullptr) RecreateAgent();

    if (agentId == -1) return;

    const dtCrowdAgent* ag = crowd->getAgent(agentId);

    if (!ag || !ag->active) return;

    float3 newPos;

    if (isPaused)
    {
        newPos               = frozenPosition;

        dtCrowdAgent* editAg = crowd->getEditableAgent(agentId);

        if (editAg)
        {
            editAg->npos[0] = frozenPosition.x;
            editAg->npos[1] = frozenPosition.y;
            editAg->npos[2] = frozenPosition.z;

            editAg->vel[0] = editAg->vel[1] = editAg->vel[2] = 0.f;
            editAg->nvel[0] = editAg->nvel[1] = editAg->nvel[2] = 0.f;
            editAg->dvel[0] = editAg->dvel[1] = editAg->dvel[2] = 0.f;
        }
    }
    else newPos = float3(ag->npos[0], ag->npos[1], ag->npos[2]);

    // float4x4 transform = parent->GetLocalTransform();
    if (!freeMovement)
        parent->SetLocalPosition(newPos - parent->GetParentGlobalTransform().TranslatePart()); // Change parent position

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
    }
}

void AIAgentComponent::RenderDebug(float deltaTime)
{
}

void AIAgentComponent::RenderEditorInspector()
{
    Component::RenderEditorInspector();

    ImGui::SeparatorText("AIAgent Component");

    if (ImGui::DragFloat("Speed", &defaultSpeed, 0.1f, 0.1f, 200.f, "%.2f")) RecreateAgent();
    if (ImGui::DragFloat("Acceleration", &defaultAcceleration, 0.1f, 0.1f, 200.f, "%.2f")) RecreateAgent();
    if (ImGui::DragFloat("Radius", &radius, 0.1f, 0.1f, 200.f, "%.2f")) RecreateAgent();
    if (ImGui::DragFloat("Height", &height, 0.1f, 0.1f, 200.f, "%.2f")) RecreateAgent();

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

void AIAgentComponent::Clone(const Component* other)
{
    if (other->GetType() == ComponentType::COMPONENT_AIAGENT)
    {
        const AIAgentComponent* otherAIAgent = static_cast<const AIAgentComponent*>(other);
        enabled                              = otherAIAgent->enabled;
        wasEnabled                           = otherAIAgent->wasEnabled;

        defaultSpeed                         = otherAIAgent->defaultSpeed;
        defaultAcceleration                  = otherAIAgent->defaultAcceleration;
        radius                               = otherAIAgent->radius;
        height                               = otherAIAgent->height;
        agentId                              = -1;
        maxAngularSpeed                      = otherAIAgent->maxAngularSpeed;

        isRadians                            = otherAIAgent->isRadians;
    }
    else
    {
        GLOG("It is not possible to clone a component of a different type!");
    }
}

// OPTIONAL TODO save agents as a resource in the future.
void AIAgentComponent::Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const
{
    Component::Save(targetState, allocator);

    targetState.AddMember("Speed", defaultSpeed, allocator);
    targetState.AddMember("Acceleration", defaultAcceleration, allocator);
    targetState.AddMember("Radius", radius, allocator);
    targetState.AddMember("Height", height, allocator);
    targetState.AddMember("MaxAngularSpeed", maxAngularSpeed, allocator);
    targetState.AddMember("isRadians", isRadians, allocator);
}

// finds closest navmesh walkable triangle.
bool AIAgentComponent::SetPathNavigation(const math::float3& destination, bool move)
{
    if (agentId == -1) return false;

    PathfinderModule* pathfinder = App->GetPathfinderModule();
    dtNavMeshQuery* navQuery     = pathfinder->GetNavQuery();
    if (!navQuery) return false;

    // Prepare for finding the nearest poly
    dtQueryFilter filter;
    float extents[3] = {2.0f, 4.0f, 2.0f}; // bounding box for the search area
    float nearestPoint[3];
    dtPolyRef targetRef;

    dtStatus status = navQuery->findNearestPoly(destination.ptr(), extents, &filter, &targetRef, nearestPoint);
    if (dtStatusFailed(status) || targetRef == 0)
    {
        // GLOG("Failed to find valid target poly for movement.");
        return false;
    }

    if (!move) return true;

    // Request move to destination

    if (lookForward)
    {
        const float3 nextPos = parent->GetGlobalTransform().TranslatePart() +
                               (parent->GetGlobalTransform().TranslatePart() - previousPos).Normalized();
        LookAtMovement(nextPos, App->GetGameTimer()->GetDeltaTime() / 1000.0f);
    }
    bool result = pathfinder->GetCrowd()->requestMoveTarget(agentId, targetRef, destination.ptr());

    if (!result)
    {
        GLOG("Crowd agent failed to request movement.");
        return false;
    }

    previousPos = parent->GetGlobalTransform().TranslatePart();
    return true;
}

void AIAgentComponent::PauseMovement()
{
    if (isPaused || agentId == -1) return;

    dtCrowd* crowd   = App->GetPathfinderModule()->GetCrowd();
    dtCrowdAgent* ag = crowd ? crowd->getEditableAgent(agentId) : nullptr;

    if (!ag) return;

    restoredSpeed              = ag->params.maxSpeed;
    restoredAccel              = ag->params.maxAcceleration;
    restoreAngular             = maxAngularSpeed;

    ag->params.maxSpeed        = 0.0f;
    ag->params.maxAcceleration = 0.0f;
    currentSpeed               = 0.0f;
    currentAngularSpeed        = 0.0f;

    crowd->resetMoveTarget(agentId);

    frozenPosition = parent->GetGlobalTransform().TranslatePart();

    isPaused       = true;
}

void AIAgentComponent::ResumeMovement()
{
    if (!isPaused || agentId == -1) return;

    dtCrowdAgent* ag = App->GetPathfinderModule()->GetCrowd()->getEditableAgent(agentId);
    if (!ag) return;

    ag->params.maxSpeed        = restoredSpeed;
    ag->params.maxAcceleration = restoredAccel;
    currentSpeed               = restoredSpeed;
    currentAngularSpeed        = restoreAngular;

    isPaused                   = false;
}

void AIAgentComponent::AddToCrowd()
{
    if (agentId != -1)
    {
        GLOG("Failed to load AI agent. Duplicate AI agent.");
        return;
    }

    currentSpeed        = defaultSpeed;
    currentAcceleration = defaultAcceleration;
    agentId             = App->GetPathfinderModule()->CreateAgent(
        parent->GetGlobalTransform().TranslatePart(), radius, height, currentSpeed, currentAcceleration
    );
    currentAngularSpeed = maxAngularSpeed;
    if (agentId != -1)
    {
        App->GetPathfinderModule()->AddAIAgentComponent(agentId, this);
    }
}

void AIAgentComponent::RecreateAgent()
{
    if (agentId != -1)
    {
        App->GetPathfinderModule()->RemoveAgent(agentId);
        agentId = -1;
    }

    AddToCrowd();
}

void AIAgentComponent::LookAtMovement(const float3& targetPos, float deltaTime)
{
    const float3 selfPos = parent->GetGlobalTransform().TranslatePart();
    float3 desired       = targetPos - selfPos;
    desired.y            = 0.0f;

    if (desired.LengthSq() < 0.0001f) return;
    desired.Normalize();

    const float4x4& localTransform = parent->GetLocalTransform();
    float3 forward                 = parent->GetGlobalTransform().WorldZ();
    forward.y                      = 0.0f;
    forward.Normalize();

    float angle   = atan2(forward.Cross(desired).y, forward.Dot(desired));

    float maxStep = currentAngularSpeed * deltaTime;
    angle         = std::clamp(angle, -maxStep, maxStep);

    if (fabs(angle) < 0.0001f) return;

    const float4x4 rotated = localTransform * float4x4::FromEulerXYZ(0.0f, angle, 0.0f);
    parent->SetLocalTransform(rotated);

    // const float4x4 rotY      = float4x4::FromEulerXYZ(0.0f, angle, 0.0f);
    // const float4x4 newGlobal = parent->GetGlobalTransform() * rotY;
    //
    // const float4x4 newlocal  = parent->GetParentGlobalTransform().Transposed() * newGlobal;
    //
    // parent->SetLocalTransform(newlocal);
    // parent->UpdateTransformForGOBranch();
}

void AIAgentComponent::SetSpeed(const float newSpeed, const float newAcceleration)
{
    dtCrowdAgent* agent           = App->GetPathfinderModule()->GetCrowd()->getEditableAgent(agentId);
    currentSpeed                  = newSpeed;
    currentAcceleration           = newAcceleration;
    agent->params.maxSpeed        = newSpeed;
    agent->params.maxAcceleration = newAcceleration;

    App->GetPathfinderModule()->GetCrowd()->resetMoveTarget(agentId);

    if (newSpeed == 0.0f)
    {
        frozenPosition = parent->GetGlobalTransform().TranslatePart();
        isPaused       = true;
    }
}

void AIAgentComponent::SetAngularSpeed(const float newAngular)
{
    currentAngularSpeed = newAngular;
}

void AIAgentComponent::ResetSpeed()
{
    dtCrowdAgent* agent           = App->GetPathfinderModule()->GetCrowd()->getEditableAgent(agentId);
    currentSpeed                  = defaultSpeed;
    currentAcceleration           = defaultAcceleration;
    agent->params.maxSpeed        = defaultSpeed;
    agent->params.maxAcceleration = defaultAcceleration;

    isPaused                      = false;
}

void AIAgentComponent::SetPosition(const float3& newPos)
{
    isPaused                     = false;

    PathfinderModule* pathfinder = App->GetPathfinderModule();
    dtNavMeshQuery* navQuery     = pathfinder->GetNavQuery();
    if (!navQuery) return;

    // Prepare for finding the nearest poly
    dtQueryFilter filter;
    float extents[3] = {2.0f, 4.0f, 2.0f}; // bounding box for the search area
    float nearestPoint[3];
    dtPolyRef targetRef;

    dtStatus status     = navQuery->findNearestPoly(newPos.ptr(), extents, &filter, &targetRef, nearestPoint);

    dtCrowdAgent* agent = App->GetPathfinderModule()->GetCrowd()->getEditableAgent(agentId);
    agent->npos[0]      = nearestPoint[0];
    agent->npos[1]      = nearestPoint[1];
    agent->npos[2]      = nearestPoint[2];

    parent->SetLocalPosition(
        float3(nearestPoint[0], nearestPoint[1], nearestPoint[2]) - parent->GetParentGlobalTransform().TranslatePart()
    );
}

void AIAgentComponent::ResetAngularSpeed()
{
    currentAngularSpeed = maxAngularSpeed;
}

unsigned int AIAgentComponent::GetClosestPointInNavmesh(
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

    // if (!dtStatusSucceed(status) || newRef == 0) return status;  // If unexpected crash, maybe this is needed

    float closest[3] = {};
    status           = navMeshQuery->closestPointOnPoly(newRef, searchPos.ptr(), closest, &posOverPoly);
    closestPoint     = {closest[0], closest[1], closest[2]};

    return status;
}

void AIAgentComponent::MoveTo(float distance, float3 rotateDirection)
{
    float deltaTime          = App->GetGameTimer()->GetDeltaTime() / 1000.0f;
    const float3& currentPos = parent->GetGlobalTransform().TranslatePart();
    const float3 offsetXZ    = rotateDirection * distance * deltaTime;
    const float3 desiredPos  = currentPos + offsetXZ;

    const float3 searchArea  = {1.0f, 1.0f, 1.0f};
    float3 closestPoint      = float3::zero;
    bool posOverPoly         = false;
    dtStatus status          = GetClosestPointInNavmesh(desiredPos, searchArea, posOverPoly, closestPoint);

    if (!dtStatusSucceed(status)) return;

    // Prevent huge changes in the y pos
    if (fabs(closestPoint.y - currentPos.y) > 0.5f) return;

    SetPosition(closestPoint - parent->GetParentGlobalTransform().TranslatePart());
}

void AIAgentComponent::StartDash(const float dashDistance, const float3& rotateDirection, float dashDuration)
{
    isDashing               = true;

    const float3 currentPos = parent->GetGlobalTransform().TranslatePart();
    const float3 finalPos   = currentPos + rotateDirection * dashDistance;
    dashDirection           = rotateDirection;
    dashSpeed               = dashDistance / dashDuration;
    dashTimeRemaining       = dashDuration;

    GLOG("FinalPosition: %.2f, %.2f, %.2f", finalPos.x, finalPos.y, finalPos.z);

    const float3 searchArea = {0.2f, 30.0f, 0.2f};
    float3 closestPoint     = float3::zero;
    bool posOverPoly        = false;
    dtStatus status         = GetClosestPointInNavmesh(finalPos, searchArea, posOverPoly, closestPoint);
    dashToNavmesh           = posOverPoly && closestPoint.y <= finalPos.y + 0.2f;
    GLOG("Dash to navmesh? %d", dashToNavmesh);
}

void AIAgentComponent::Dash(float deltaTime)
{
    const float3 currentPos = parent->GetGlobalTransform().TranslatePart();

    const float3 dashOffset = dashDirection * dashSpeed * deltaTime;
    float3 desiredPos       = currentPos + dashOffset;
    const float3 searchArea = {62.5f * deltaTime, std::max(0.4f, 25.0f * deltaTime), 62.5f * deltaTime};
    bool posOverPoly        = false;
    float3 closestPoint     = float3::zero;

    dtStatus status         = GetClosestPointInNavmesh(desiredPos, searchArea, posOverPoly, closestPoint);

    if (!dashToNavmesh || (posOverPoly && dashToNavmesh)) desiredPos = closestPoint;

    parent->SetLocalPosition(desiredPos - parent->GetGlobalTransform().TranslatePart());
    dashTimeRemaining -= deltaTime;

    if (dashTimeRemaining > 0.05f && preciseDash)
    {
        // Check if the end of the remaining dash is inside the navmesh in case we are sliding next to the edge
        const float3 currentPos = parent->GetGlobalTransform().TranslatePart();
        const float3 finalPos   = currentPos + dashDirection * dashSpeed * dashTimeRemaining;

        const float3 searchArea = {12.5f * deltaTime, std::max(1875.0f * deltaTime, 30.0f), 12.5f * deltaTime};
        float3 closestPoint     = float3::zero;
        bool posOverPoly        = false;
        dtStatus status         = GetClosestPointInNavmesh(finalPos, searchArea, posOverPoly, closestPoint);
        dashToNavmesh           = posOverPoly && closestPoint.y <= finalPos.y + 0.2f;
    }

    if (dashTimeRemaining <= 0.0f) isDashing = false;
}
