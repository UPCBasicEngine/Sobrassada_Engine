#pragma once

#include "Module.h"

#include <map>

class BatchManager;
class GeometryBatch;
class Resource;

class ResourcesModule : public Module
{
  public:
    ResourcesModule();
    ~ResourcesModule() override;
    // update_status PostUpdate(float deltaTime) override;
    bool ShutDown() override;

    Resource* RequestResource(UID uid);
    void ReleaseResource(const Resource* resource);
    void UnloadAllResources();

    BatchManager* GetBatchManager() { return batchManager; }

  private:
    Resource* CreateNewResource(UID uid);

  private:
    std::map<UID, Resource*> resources;
    BatchManager* batchManager = nullptr;
};
