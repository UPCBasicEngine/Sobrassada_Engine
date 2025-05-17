#pragma once

#include "Globals.h"

#include "Math/float3.h"
#include <list>
#include <vector>

class ResourceMaterial;
class ResourceTexture;
class BillboardComponent;

class Billboard
{
  public:
    Billboard(float width, float height);
    ~Billboard();

    void UpdateWidth(float newWidth);
    void UpdateHeight(float newHeight);
    void UpdateMaterial(UID newMaterialUID);
    void UpdateTexture(UID newTextureUID);
    void UpdateLockPitch(bool newLock);
    void UpdateUseTexture(bool newTexture);

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

    bool useTexture            = false;
    ResourceMaterial* material = nullptr;
    ResourceTexture* texture   = nullptr;

    std::list<BillboardComponent*> instanceComponents;
    std::vector<float3> instancePositions;
    bool reloadPositions = false;
};
