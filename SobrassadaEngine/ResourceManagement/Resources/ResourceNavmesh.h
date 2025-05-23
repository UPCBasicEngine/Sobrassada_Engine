#pragma once

#include "Resource.h"

class NavMeshConfig;
class ResourceMesh;
class dtNavMesh;
class dtNavMeshQuery;
struct rcConfig;
class rcContext;
struct rcHeightfield;
struct rcCompactHeightfield;
struct rcContourSet;
struct rcPolyMesh;
struct rcPolyMeshDetail;
class ResourceMesh;

struct Vertex;

enum SamplePolyAreas
{
    SAMPLE_POLYAREA_GROUND,
    SAMPLE_POLYAREA_WATER,
    SAMPLE_POLYAREA_ROAD,
    SAMPLE_POLYAREA_DOOR,
    SAMPLE_POLYAREA_GRASS,
    SAMPLE_POLYAREA_JUMP
};

enum SamplePolyFlags
{
    SAMPLE_POLYFLAGS_WALK     = 0x01,  // Ability to walk (ground, grass, road)
    SAMPLE_POLYFLAGS_SWIM     = 0x02,  // Ability to swim (water).
    SAMPLE_POLYFLAGS_DOOR     = 0x04,  // Ability to move through doors.
    SAMPLE_POLYFLAGS_JUMP     = 0x08,  // Ability to jump.
    SAMPLE_POLYFLAGS_DISABLED = 0x10,  // Disabled polygon
    SAMPLE_POLYFLAGS_ALL      = 0xffff // All abilities.
};

class ResourceNavMesh : public Resource
{

  public:
    ResourceNavMesh(UID uid, const std::string& name);
    ~ResourceNavMesh() override;

    bool BuildNavMesh(
        const std::vector<std::pair<const ResourceMesh*, const float4x4&>>& meshes, const float minPoint[3],
        const float maxPoint[3], const NavMeshConfig& navconf
    );

    void CreateDetourData(const rcPolyMesh* pmesh, const rcPolyMeshDetail* dmesh, const rcConfig& config);
    void SetDetourNavMesh(dtNavMesh* navMesh);
    const dtNavMesh* GetDetourNavMesh() const { return navMesh; }
    dtNavMesh* GetDetourNavMesh()
    {
        if (navMesh != nullptr) return navMesh;
        else return nullptr;
    }
  private:
    dtNavMesh* navMesh = nullptr;
};