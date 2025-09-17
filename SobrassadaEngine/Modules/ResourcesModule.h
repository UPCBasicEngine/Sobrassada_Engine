#pragma once

#include "Module.h"

#include <map>

class BatchManager;
class GeometryBatch;
class Resource;

class SOBRASADA_API_ENGINE ResourcesModule : public Module
{
  public:
    ResourcesModule();
    ~ResourcesModule() override;
    // update_status PostUpdate(float deltaTime) override;
    bool ShutDown() override;

    Resource* RequestResource(UID uid);
    void ReleaseResource(const Resource* resource);
    void ReleaseResource(UID resourceUID);
    void ForceUnloadResource(UID resourceUID);
    void UnloadAllResources();

    BatchManager* GetBatchManager() { return batchManager; }

  private:
    Resource* CreateNewResource(UID uid);

  private:
    std::map<UID, Resource*> resources;
    BatchManager* batchManager = nullptr;
};
