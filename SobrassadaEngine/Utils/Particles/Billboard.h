#pragma once

#include "Globals.h"

#include "Math/float3.h"
#include <list>

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

    void CreateVertexBufferObject();

    std::list<BillboardComponent*>::iterator AddComponent(BillboardComponent* newBillboard);
    void RemoveComponent(std::list<BillboardComponent*>::iterator newBillboard);

  private:
    float width                = 1;
    float height               = 1;
    bool lockPitch             = false;
    unsigned int vbo           = 0;
    ResourceMaterial* material = nullptr;
    std::list<BillboardComponent*> instanceComponents;
};
