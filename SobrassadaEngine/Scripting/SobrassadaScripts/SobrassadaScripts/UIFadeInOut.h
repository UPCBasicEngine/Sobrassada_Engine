#pragma once

#include "Script.h"

#include "Math/float2.h"

class ImageComponent;
class ResourceTexture;

class UIFadeInOut : public Script
{
  public:
    UIFadeInOut(GameObject* parent);
    ~UIFadeInOut() override;

    bool Init() override;
    void Update(float deltaTime) override;
    void Render(float deltaTime, CameraComponent* cameraComp) override;
    void Reset() override;

    void FadeIn();
    void FadeOut();

    bool GetFadingIn() const { return isFadingIn; }
    bool GetFadingOut() const { return isFadingOut; }

    float GetFadeInDuration() const { return fadeInDuration; }

  private:
    unsigned int shaderProgram  = 0;

    unsigned int vao            = 0;
    unsigned int vbo            = 0;
    unsigned int texture        = 0;

    ImageComponent* imageComp   = nullptr;

    bool fadeInAuto             = false;
    bool fadeOutAuto            = false;
    bool isFadingIn             = false;
    bool isFadingOut            = false;
    bool isVisible              = false;
    bool startVisible           = true;

    float timer                 = 0.0f;
    float fadeInStart           = 0.0f;
    float fadeInDuration        = 0.0f;
    float fadeInOpacity         = 1.0f;
    float fadeOutStart          = 0.0f;
    float fadeOutDuration       = 0.0f;
    float fadeOutOpacity        = 0.0f;

    bool hasFadedIn             = false;
    bool hasFadedOut            = false;
    float automaticFadeInStart  = -1.0f;
    float automaticFadeOutStart = -1.0f;
};
