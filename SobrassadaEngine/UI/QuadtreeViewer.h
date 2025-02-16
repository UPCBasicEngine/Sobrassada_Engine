#pragma once

#include "Geometry/Frustum.h"
#include "Math/float4x4.h"
#include "Geometry/AABB2D.h"
#include <vector>

class Framebuffer;
class Quadtree;
class MockGameObject;

class QuadtreeViewer
{
  public:
    QuadtreeViewer();
    ~QuadtreeViewer();

    void Render(bool &renderBoolean);

  private:
    void ChangeCameraSize(float width, float height);
    void CreateQueryAreaLines(const AABB &queryArea, std::vector<float4> &queryAreaLines) const;
    void CreateGameObjectsAreaLines(std::vector<float4> &elementsAreaLines) const;

  private:
    Framebuffer *framebuffer;
    Quadtree *quadtree;
    float cameraSizeScaleFactor = 10.f;

    Frustum camera;
    float4x4 viewMatrix;
    float4x4 projectionMatrix;

    std::vector<MockGameObject*> gameObjects;
};
