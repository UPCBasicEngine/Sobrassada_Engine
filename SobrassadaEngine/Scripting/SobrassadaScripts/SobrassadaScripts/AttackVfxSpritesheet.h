#pragma once

#include "Script.h"

#include "Math/float2.h"

class MeshComponent;
class ResourceTexture;

class AttackVfxSpritesheet : public Script
{
  public:
    AttackVfxSpritesheet(GameObject* parent);
    ~AttackVfxSpritesheet() override;

    bool Init() override;
    void Update(float deltaTime) override;
    void Render(float deltaTime, CameraComponent* cameraComp) override;

    void Reset() override;
    void UpdateSprite(float deltaTime);

    const bool AlmostFinished(int row, int col) const;
    const bool Finished() const { return finished; }
    bool IsInitialized() const { return isInitialized; }

  private:
    void ResetUVs(ResourceTexture* tex);

    unsigned int shaderProgram     = 0;
    bool isInitialized             = false;

    unsigned int vao               = 0;
    unsigned int vbo               = 0;
    unsigned int ebo               = 0;

    unsigned int indexCount        = 0;
    bool isDoubleSided             = false;
    float updateRate               = 1.0f;
    float timer                    = 0.0f;

    bool isRowMajor                = false;
    float cellHeight               = 0.1f;
    float cellWidth                = 0.1f;
    float4 uvRange                 = float4::zero;

    MeshComponent* meshComp        = nullptr;

    ResourceTexture* otherImage    = nullptr;
    UID otherImageUID              = 0;
    UID otherImageBindlessUID      = 0;

    float2 step                 = float2::zero;

    bool isOneShot                 = false;
    bool isAdditive                = false;

    int variationsToUse            = 0;
    ResourceTexture* variations[4] = {nullptr};
    UID variationsUID1;
    UID variationsUID2;
    UID variationsUID3;
    UID variationsUID4;
    UID variationsBindlessUID[4];

    UID currentImageUID = 0;

    bool onlyOnce               = false;
    bool finished               = false;

    bool useRowCol              = false;
    int rows                    = 1;
    int cols                    = 1;

    bool useAnimDuration        = false;
    float animationDuration     = 0.1f;
};
