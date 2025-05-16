#pragma once

#include "Globals.h"

#include "Math/float3.h"
#include <list>
#include <vector>

class ResourceMaterial;
class BillboardComponent;

class Billboard
{
  public:
    Billboard(float width, float height);
    ~Billboard();

    void UpdateWidth(float newWidth);
    void UpdateHeight(float newHeight);
    void UpdateMaterial(UID newMaterial);

    void Render();

    void AddComponent(BillboardComponent* newBillboard);
    void RemoveComponent(std::list<BillboardComponent*>::iterator newBillboard);
    void CheckReloadPositions();

    void SetReloadPositions() { reloadPositions = true; };

  private:
    void CreateVertexBufferObject();
    void UpdatePositionsVbo();

  private:
    unsigned int positionsVbo  = 0;
    unsigned int vbo           = 0;

    float width                = 1;
    float height               = 1;
    bool lockPitch             = false;

    ResourceMaterial* material = nullptr;
    std::list<BillboardComponent*> instanceComponents;
    std::vector<float3> instancePositions;
    bool reloadPositions = false;
};
