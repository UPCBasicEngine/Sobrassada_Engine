#include "PathfinderModule.h"

#include "Application.h"
#include "CameraModule.h"
#include "GameObject.h"
#include "GameTimer.h"
#include "LibraryModule.h"
#include "MetaNavmesh.h"
#include "NavmeshImporter.h"
#include "ResourceNavmesh.h"
#include "ResourcesModule.h"
#include "Scene.h"
#include "SceneModule.h"
#include "Standalone/AIAgentComponent.h"
#include "Standalone/MeshComponent.h"

#include "Geometry/Plane.h"
#include "ImGui.h"

#include "DetourCrowd.h"
#include "DetourNavMesh.h"
#include "DetourNavMeshQuery.h"

PathfinderModule::PathfinderModule()
{
}

bool PathfinderModule::Init()
{
    if (!crowd) crowd = dtAllocCrowd();

    return true;
}

PathfinderModule::~PathfinderModule()
{
    dtFreeCrowd(crowd);
    if (navQuery)
    {
        dtFreeNavMeshQuery(navQuery);
        navQuery = nullptr;
    }
    ClearNavMesh();
}

update_status PathfinderModule::Update(float deltaTime)
{
    if (!App->GetSceneModule()->GetInPlayMode()) return UPDATE_CONTINUE;

    if (crowd->getAgentCount() > 0)
    {
        if (App->GetSceneModule()->GetInPlayMode()) deltaTime = App->GetGameTimer()->GetDeltaTime() / 1000.0f;
        crowd->update(deltaTime, nullptr);
    }

    return UPDATE_CONTINUE;
}

// All ai agent components will call this to add themselves to crowd
int PathfinderModule::CreateAgent(
    const float3& position, const float radius, const float height, const float speed, const float acceleration
)
{

    dtCrowdAgentParams params;
    memset(&params, 0, sizeof(params));
    params.radius                = radius;
    params.height                = height;
    params.maxSpeed              = speed;
    params.maxAcceleration       = acceleration;
    params.collisionQueryRange   = radius * 12.0f;
    params.pathOptimizationRange = radius * 30.0f;
    params.updateFlags           = DT_CROWD_ANTICIPATE_TURNS | DT_CROWD_OBSTACLE_AVOIDANCE | DT_CROWD_SEPARATION;
    params.obstacleAvoidanceType = 3;
    params.separationWeight      = 2.0f;

    if (!crowd) return 0;

    return crowd->addAgent(position.ptr(), &params);
}

void PathfinderModule::RemoveAgent(int agentId)
{
    if (agentId < 0 || agentId >= crowd->getAgentCount()) return;

    const dtCrowdAgent* agent = crowd->getAgent(agentId);
    if (!agent || !agent->active) return;

    crowd->removeAgent(agentId);
    RemoveAIAgentComponent(agentId);
}

void PathfinderModule::InitQuerySystem()
{
    if (!navmesh)
    {
        GLOG("[Error] No navmesh assigned for query system");
        return;
    }

    // Free previous navQuery
    if (navQuery)
    {
        dtFreeNavMeshQuery(navQuery);
        navQuery = nullptr;
    }

    // Allocate and initialize new query
    navQuery = dtAllocNavMeshQuery();
    if (navQuery && navmesh->GetDetourNavMesh())
    {
        navQuery->init(navmesh->GetDetourNavMesh(), maxNodes);
    }
    else
    {
        GLOG("Failed to initialize navQuery.");
    }

    if (crowd)
    {
        dtFreeCrowd(crowd);
        crowd = nullptr;
    }

    crowd = dtAllocCrowd();
    if (crowd && navmesh->GetDetourNavMesh())
    {
        crowd->init(maxAgents, maxAgentRadius, navmesh->GetDetourNavMesh());
    }
    else
    {
        GLOG("Failed to initialize Pathfinder crowd system.");
    }
}

void PathfinderModule::ResetNavmesh()
{
    navmesh = new ResourceNavMesh(GenerateUID(), DEFAULT_NAVMESH_NAME);
}

// Currently called by clicking in the game, but any float3 will move the agents there. ONLY WORKS IN PLAY MODE
void PathfinderModule::HandleClickNavigation()
{
    if (!clickNavigationEnabled || !App->GetSceneModule()->GetInPlayMode()) return; // from UI

    LineSegment ray = App->GetCameraModule()->CastCameraRay();

    float3 hitPoint;

    RaycastToGround(ray, hitPoint);

    for (auto& pair : agentComponentMap)
    {
        AIAgentComponent* comp = pair.second;
        if (comp != nullptr) comp->SetPathNavigation(hitPoint);
    }
}

void PathfinderModule::GetClickNavigation()
{
    if (!clickNavigationEnabled || App->GetSceneModule()->GetInPlayMode()) return; // from UI

    LineSegment ray = App->GetCameraModule()->CastCameraRay();

    float3 hitPoint;

    RaycastToGround(ray, hitPoint);

    GLOG("Navmesh point: %.2f, %.2f, %.2f", hitPoint.x, hitPoint.y, hitPoint.z);
}

bool PathfinderModule::RaycastToGround(const LineSegment& ray, float3& outHitPoint)
{
    Plane groundPlane(float3::unitY, 0.0f); // Ground at Y = 0
    float distance = 0.0f;

    if (ray.Intersects(groundPlane, &distance))
    {
        outHitPoint = ray.GetPoint(distance);
        return true;
    }

    return false;
}

void PathfinderModule::RenderCrowdEditor()
{
    ImGui::Checkbox("Navigation Enabled", &clickNavigationEnabled);
}

void PathfinderModule::SaveNavMesh(const std::string& name)
{
    if (!navmesh) // todo check if it's built maybe?
    {
        GLOG("Cannot save: NavMesh not built.");
        return;
    }

    const UID uid = NavmeshImporter::SaveNavmesh(name.c_str(), navmesh, navconf);
    App->GetSceneModule()->GetScene()->SetNavmeshUID(uid);

    // GLOG("NavMesh saved with UID: %u", uid);
}

void PathfinderModule::LoadNavMesh(const std::string& name)
{
    ClearNavMesh();

    const UID navmeshUID = App->GetLibraryModule()->GetNavmeshUID(name);
    if (navmeshUID == 0)
    {
        GLOG("Navmesh '%s' not found in LibraryModule.", name.c_str());
        return;
    }

    ResourceNavMesh* loadedNavmesh = NavmeshImporter::LoadNavmesh(navmeshUID);
    if (!loadedNavmesh)
    {
        GLOG("Failed to load navmesh binary for UID %llu.", navmeshUID);
        return;
    }

    navconf.LoadFromMeta(navmeshUID);
    navmesh = loadedNavmesh;

    InitQuerySystem();

    // GLOG("Navmesh '%s' successfully loaded and set.", name.c_str());
}

void PathfinderModule::AddAIAgentComponent(int agentId, AIAgentComponent* comp)
{
    agentComponentMap[agentId] = comp;
}

void PathfinderModule::RemoveAIAgentComponent(int agentId)
{
    agentComponentMap.erase(agentId);
}
AIAgentComponent* PathfinderModule::GetComponentFromAgentId(int agentId)
{
    auto it = agentComponentMap.find(agentId);
    return (it != agentComponentMap.end()) ? it->second : nullptr;
}

void PathfinderModule::CreateNavMesh()
{
    ClearNavMesh();

    UID navUID = GenerateUID();

    navmesh    = new ResourceNavMesh(navUID, "RuntimeNavMesh");

    std::vector<std::pair<const ResourceMesh*, const float4x4&>> meshes;
    float minPos[3]                                         = {FLT_MAX, FLT_MAX, FLT_MAX};
    float maxPos[3]                                         = {FLT_MIN, FLT_MIN, FLT_MIN};

    const std::unordered_map<UID, GameObject*>& gameObjects = App->GetSceneModule()->GetScene()->GetAllGameObjects();

    if (!gameObjects.empty())
    {
        for (const auto& pair : gameObjects)
        {
            GameObject* gameObject = pair.second;
            if (!gameObject || !gameObject->IsGloballyEnabled() || !gameObject->IsNavMeshValid()) continue;
            {
                const MeshComponent* meshComponent = gameObject->GetComponent<MeshComponent*>();
                const float4x4& globalMatrix       = gameObject->GetGlobalTransform();

                if (meshComponent && meshComponent->GetEnabled())
                {
                    const ResourceMesh* resourceMesh = meshComponent->GetResourceMesh();
                    if (resourceMesh == nullptr) continue; // If a meshComponent has no mesh attached, ignore it

                    const AABB& aabb = gameObject->GetGlobalAABB();

                    minPos[0]        = std::min(minPos[0], aabb.minPoint.x);
                    minPos[1]        = std::min(minPos[1], aabb.minPoint.y);
                    minPos[2]        = std::min(minPos[2], aabb.minPoint.z);

                    maxPos[0]        = std::max(maxPos[0], aabb.maxPoint.x);
                    maxPos[1]        = std::max(maxPos[1], aabb.maxPoint.y);
                    maxPos[2]        = std::max(maxPos[2], aabb.maxPoint.z);

                    meshes.push_back({resourceMesh, globalMatrix});
                }
            }
        }
    }

    if (meshes.size() == 0)
    {
        GLOG("[WARNING] Trying to create NavMesh but no meshes are found in the scene");
        return;
    }
    navmesh->BuildNavMesh(meshes, minPos, maxPos, navconf);

    InitQuerySystem();
}

void PathfinderModule::ClearNavMesh()
{
    if (navmesh)
    {
        delete navmesh;
        navmesh = nullptr;
    }
}