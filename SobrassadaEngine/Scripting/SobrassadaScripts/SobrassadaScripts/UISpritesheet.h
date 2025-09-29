#pragma once

#include "Script.h"

#include "Math/float2.h"

class ImageComponent;
class ResourceTexture;

class UISpritesheet : public Script
{
  public:
    UISpritesheet(GameObject* parent);
    ~UISpritesheet() override;

    bool Init() override;
    void Update(float deltaTime) override;
    void Render(float deltaTime, CameraComponent* cameraComp) override;
    void Reset() override;

    void UpdateSprite(float deltaTime);

  private:
    unsigned int shaderProgram   = 0;

    unsigned int vao             = 0;
    unsigned int vbo             = 0;
    unsigned int texture         = 0;

    bool disableDefaultImage     = true;
    float updateRate             = 1.0f;
    float timer                  = 0.0f;

    bool isRowMajor              = false;
    float cellHeight             = 0.1f;
    float cellWidth              = 0.1f;
    float4 uvRange               = float4::zero;

    ImageComponent* imageComp    = nullptr;
    ResourceTexture* spritesheet = nullptr;
    UID spritesheetUID           = 0;
    UID spritesheetBindlessUID   = 0;
};
