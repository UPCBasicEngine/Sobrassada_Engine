#include "NavmeshImporter.h"

#include "Application.h"
#include "DetourNavMesh.h"
#include "FileSystem.h"
#include "LibraryModule.h"
#include "MetaNavmesh.h"
#include "NavMeshConfig.h"
#include "ProjectModule.h"
#include "ResourceNavmesh.h"

struct NavMeshSetHeader
{
    int magic;
    int version;
    dtNavMeshParams params;
};

UID NavmeshImporter::SaveNavmesh(const char* name, const ResourceNavMesh* resource, const NavMeshConfig& config)
{

    const int NAVMESHSET_MAGIC   = 'S' << 24 | 'O' << 16 | 'B' << 8 | 'R';
    const int NAVMESHSET_VERSION = 1;

    const dtNavMesh* navmesh     = resource->GetDetourNavMesh();
    if (!navmesh) return 0;

    const dtNavMeshParams* params = navmesh->getParams();

    // Get raw navmesh data (we assume 1 tile, single data block)
    const dtMeshTile* tile        = navmesh->getTileAt(0, 0, 0);

    if (!tile || !tile->data || tile->dataSize == 0) return 0;

    // Fill header
    NavMeshSetHeader header;
    header.magic   = NAVMESHSET_MAGIC;
    header.version = NAVMESHSET_VERSION;
    memcpy(&header.params, params, sizeof(dtNavMeshParams));

    // Allocate buffer
    const size_t totalSize = sizeof(NavMeshSetHeader) + tile->dataSize;

    char* fileBuffer       = new char[totalSize];
    char* cursor           = fileBuffer;

    memcpy(cursor, &header, sizeof(NavMeshSetHeader));
    cursor += sizeof(NavMeshSetHeader);
    memcpy(cursor, tile->data, tile->dataSize);

    // Generate UID and file path
    UID navmeshUID             = GenerateUID();
    navmeshUID                 = App->GetLibraryModule()->AssignFiletypeUID(navmeshUID, FileType::Navmesh);

    UID prefix                 = navmeshUID / UID_PREFIX_DIVISOR;
    const std::string savePath = App->GetProjectModule()->GetLoadedProjectPath() + METADATA_PATH +
                                 std::to_string(prefix) + FILENAME_SEPARATOR + name + META_EXTENSION;
    UID finalNavmeshUID = App->GetLibraryModule()->GetUIDFromMetaFile(savePath);
    if (finalNavmeshUID == INVALID_UID) finalNavmeshUID = navmeshUID;

    const std::string metaNavPath = NAVMESH_ASSETS_PATH + std::string(name) + NAVMESH_EXTENSION;
    const std::string navPath =
        App->GetProjectModule()->GetLoadedProjectPath() + NAVMESHES_PATH + std::string(name) + NAVMESH_EXTENSION;

    // Save metadata
    MetaNavmesh meta(finalNavmeshUID, metaNavPath, config);
    meta.Save(name, metaNavPath);

    // Write binary navmesh
    const unsigned int bytesWritten =
        (unsigned int)FileSystem::Save(navPath.c_str(), fileBuffer, (unsigned int)totalSize, true);

    // Write binary in assets so its shared across Git

    const std::string assetNavmeshPath = App->GetProjectModule()->GetLoadedProjectPath() + metaNavPath;
    FileSystem::Save(assetNavmeshPath.c_str(), fileBuffer, (unsigned int)totalSize, true);

    delete[] fileBuffer;

    App->GetLibraryModule()->AddNavmesh(finalNavmeshUID, name);
    App->GetLibraryModule()->AddResource(navPath, finalNavmeshUID);
    App->GetLibraryModule()->AddName(name, finalNavmeshUID);

    //GLOG("%s saved navmesh binary.", name);

    return finalNavmeshUID;
}

ResourceNavMesh* NavmeshImporter::LoadNavmesh(const UID navmeshUID)
{
    const int NAVMESHSET_MAGIC   = 'S' << 24 | 'O' << 16 | 'B' << 8 | 'R';
    const int NAVMESHSET_VERSION = 1;

    const std::string navPath    = App->GetLibraryModule()->GetResourcePath(navmeshUID);

    char* buffer                 = nullptr;
    unsigned int size            = FileSystem::LoadForDetour(navPath.c_str(), &buffer);
    if (size == 0 || !buffer)
    {
        GLOG("Failed to load navmesh binary: %s", navPath.c_str());
        return nullptr;
    }

    char* cursor = buffer;

    // Read and verify header
    NavMeshSetHeader header;
    memcpy(&header, cursor, sizeof(NavMeshSetHeader));
    cursor += sizeof(NavMeshSetHeader);

    if (header.magic != NAVMESHSET_MAGIC || header.version != NAVMESHSET_VERSION)
    {
        GLOG("Invalid navmesh file: %s", navPath.c_str());
        delete[] buffer;
        return nullptr;
    }

    dtNavMesh* navMesh = dtAllocNavMesh();
    if (!navMesh || dtStatusFailed(navMesh->init(&header.params)))
    {
        GLOG("Failed to init Detour navmesh");
        delete[] buffer;
        return nullptr;
    }

    dtStatus status = navMesh->init(&header.params);
    if (dtStatusFailed(status))
    {
        GLOG("Failed to init Detour navmesh.");
        dtFreeNavMesh(navMesh);
        return nullptr;
    }
    unsigned char* tileData = (unsigned char*)cursor;
    int tileRef             = 0;
    status = navMesh->addTile(tileData, size - sizeof(NavMeshSetHeader), DT_TILE_FREE_DATA, 0, (dtTileRef*)&tileRef);
    if (dtStatusFailed(status))
    {
        GLOG("Failed to add tile to Detour navmesh.");
        dtFreeNavMesh(navMesh);
        return nullptr;
    }

    ResourceNavMesh* resource = new ResourceNavMesh(navmeshUID, "LoadedNavmesh");
    resource->SetDetourNavMesh(navMesh);
    //GLOG("Loaded navmesh binary: %s", navPath.c_str());

    return resource;
}

void NavmeshImporter::CopyNavmesh(
    const std::string& assetPath, const std::string& projectPath, const std::string& libraryPath, UID assetUID
)
{
    if (FileSystem::Exists(assetPath.c_str()))
    {
        std::string copylibraryPath = projectPath + NAVMESHES_PATH;
        if (FileSystem::Copy(assetPath.c_str(), copylibraryPath.c_str()))
            App->GetLibraryModule()->AddResource(libraryPath, assetUID);
    }
}
