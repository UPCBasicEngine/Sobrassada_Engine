#pragma once

#include <vector>
#include <unordered_map>
#include "glew.h"

class GeometryBatch;
class MeshComponent;
class CameraComponent;

struct UniformCache
{
    GLint isWireframe    = -1;
    GLint isAlpha        = -1;
    GLint windParameters = -1;
    GLint windUVParams   = -1;
    GLint windAmplitudes = -1;
    GLint windFrequency  = -1;
    GLint cameraBlockIdx = -1;
    bool initialized     = false;
};

class BatchManager
{
  public:
    BatchManager();
    ~BatchManager();

    void UnloadAllBatches();
    void RemoveBatch(GeometryBatch* batch);

    void LoadData();
    void Render(const std::vector<MeshComponent*>& meshesToRender, CameraComponent* camera, bool isWireframe);
    void RenderTransparent(
        const std::vector<MeshComponent*>& meshesToRender, const unsigned int program, CameraComponent* camera
    );
    void RenderShadowMap(const std::vector<MeshComponent*>& meshesToRender, unsigned int cameraUBO);
    void SwapBuffers();

    GeometryBatch* RequestBatch(const MeshComponent* mesh);

    GeometryBatch* CreateNewBatch(const MeshComponent* mesh);

  private:
    std::vector<GeometryBatch*> opaqueBatches;
    std::vector<GeometryBatch*> transparentBatches;
    std::unordered_map<GLuint, UniformCache> uniformCacheMap;
};
