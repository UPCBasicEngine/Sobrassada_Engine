#pragma once

#include "Globals.h"

class NavMeshConfig;
class ResourceNavMesh;
class dtNavMesh;

namespace NavmeshImporter
{
    UID SaveNavmesh(const char* name, const ResourceNavMesh* resource, const NavMeshConfig& config);
    ResourceNavMesh* LoadNavmesh(UID navmeshUID);
    void CopyNavmesh(
        const std::string& assetPath, const std::string& projectPath, const std::string& libraryPath, UID assetUID
    );

}; // namespace NavmeshImporter
