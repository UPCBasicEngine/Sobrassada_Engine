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

    SOBRASADA_API_ENGINE Resource* RequestResource(UID uid);
    void ReleaseResource(const Resource* resource);
    void UnloadAllResources();

    BatchManager* GetBatchManager() { return batchManager; }
    const std::map<UID, Resource*>& GetAllResources() { return resources; }

  private:
    Resource* CreateNewResource(UID uid);

  private:
    std::map<UID, Resource*> resources;
    BatchManager* batchManager = nullptr;
};
