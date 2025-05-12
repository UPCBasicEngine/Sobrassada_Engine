#pragma once

#include "Module.h"
#include "NavmeshConfig.h"
#include <unordered_map>

class dtNavMesh;
class dtNavMeshQuery;
class dtCrowd;
class ResourceNavMesh;
class AIAgentComponent;

class PathfinderModule : public Module
{
  public:
    PathfinderModule();
    ~PathfinderModule();
    update_status Update(float deltaTime) override;

    bool Init() override;
    void CreateNavMesh();
    void ClearNavMesh();
    int CreateAgent(const float3& position, const float radius, const float height, const float speed);
    dtCrowd* GetCrowd() const { return crowd; }
    dtNavMeshQuery* GetNavQuery() const { return navQuery; }
    void RemoveAgent(int agentId);
    void InitQuerySystem();
    void HandleClickNavigation(); // Perform raycast and set destination
    void GetClickNavigation();
    bool RaycastToGround(const LineSegment& ray, float3& outHitPoint);
    void RenderCrowdEditor();
    ResourceNavMesh* GetNavMesh() { return navmesh; }

    void ResetNavmesh();
    void SaveNavMesh(const std::string& name);
    void LoadNavMesh(const std::string& name);
    dtNavMeshQuery* GetDetourNavMeshQuery() const { return navQuery; }
    NavMeshConfig& GetNavMeshConfig() { return navconf; }

    void AddAIAgentComponent(int agentId, AIAgentComponent* comp);
    void RemoveAIAgentComponent(int agentId);
    AIAgentComponent* GetComponentFromAgentId(int agentId);

  private:
    dtNavMeshQuery* navQuery = nullptr;
    dtCrowd* crowd           = nullptr;
    ResourceNavMesh* navmesh = nullptr;
    const unsigned int maxAgents = 100;
    const float maxAgentRadius   = 0.5f;
    bool clickNavigationEnabled  = false;
    NavMeshConfig navconf;
    float3 outHitPoint;
    const unsigned int maxNodes = 2048;
    std::unordered_map<int, AIAgentComponent*> agentComponentMap;
};
