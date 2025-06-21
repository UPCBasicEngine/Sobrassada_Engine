#pragma once

#include <vector>

class GeometryBatch;
class MeshComponent;
class CameraComponent;

class BatchManager
{
  public:
    BatchManager();
    ~BatchManager();

    void UnloadAllBatches();
    void RemoveBatch(GeometryBatch* batch);

    void LoadData();
    void Render(const std::vector<MeshComponent*>& meshesToRender, CameraComponent* camera, bool isWireframe);
    void RenderTransparent(const std::vector<MeshComponent*>& meshesToRender, CameraComponent* camera);
    void RenderShadowMap(const std::vector<MeshComponent*>& meshesToRender, unsigned int cameraUBO);

    GeometryBatch* RequestBatch(const MeshComponent* mesh);

    GeometryBatch* CreateNewBatch(const MeshComponent* mesh);

  private:
    std::vector<GeometryBatch*> opaqueBatches;
    std::vector<GeometryBatch*> transparentBatches;
};
