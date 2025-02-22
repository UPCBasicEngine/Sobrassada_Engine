#pragma once

#include "Geometry/Frustum.h"
#include "Math/float4x4.h"
#include <list>

class Framebuffer;
class Quadtree;

struct Box;

class QaudtreeViewer
{
  public:
    QaudtreeViewer();
    ~QaudtreeViewer();

    void Render(bool &renderBoolean);

  private:
    void ChangeCameraSize(float width, float height);
    void CreateQueryAreaLines(const Box &queryArea, std::list<float4> &queryAreaLines) const;

  private:
    Framebuffer *framebuffer;
    Quadtree *quadtree;
    float cameraSizeScaleFactor = 10.f;

    Frustum camera;
    float4x4 viewMatrix;
    float4x4 projectionMatrix;
};
