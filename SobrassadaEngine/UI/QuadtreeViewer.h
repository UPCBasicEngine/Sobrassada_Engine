#pragma once

#include "Geometry/Frustum.h"
#include "Math/float4x4.h"
#include <vector>

class Framebuffer;
class Quadtree;

struct Box;

class QuadtreeViewer
{
  public:
    QuadtreeViewer();
    ~QuadtreeViewer();

    void Render(bool &renderBoolean);

  private:
    void ChangeCameraSize(float width, float height);
    void CreateQueryAreaLines(const Box &queryArea, std::vector<float4> &queryAreaLines) const;

  private:
    Framebuffer *framebuffer;
    Quadtree *quadtree;
    float cameraSizeScaleFactor = 10.f;

    Frustum camera;
    float4x4 viewMatrix;
    float4x4 projectionMatrix;
};
