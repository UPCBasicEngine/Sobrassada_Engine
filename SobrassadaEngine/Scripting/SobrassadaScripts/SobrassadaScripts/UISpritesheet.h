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

    void SetFadeOut(bool fade) { isFadeOut = fade; };

  private:
    void UpdateSprite(float deltaTime);

  private:
    unsigned int shaderProgram   = 0;

    unsigned int vao             = 0;
    unsigned int vbo             = 0;
    unsigned int texture         = 0;

    bool disableDefaultImage     = true;
    bool isOneShot               = false;
    bool isFadeOut               = false;
    float updateRate             = 1.0f;
    float timer                  = 0.0f;
    float fadeOutDuration        = 0.0f;
    float fadeOutTime            = 0.0f;
    float fadeOutStart           = 0.0f;

    bool isRowMajor              = false;
    float cellHeight             = 0.1f;
    float cellWidth              = 0.1f;
    float4 uvRange               = float4::zero;

    ImageComponent* imageComp    = nullptr;
    ResourceTexture* spritesheet = nullptr;
    UID spritesheetUID           = 0;
    UID spritesheetBindlessUID   = 0;

    float2 step                  = float2::zero;
    bool useRowCol               = false;
    int rows                     = 1;
    int cols                     = 1;
};
