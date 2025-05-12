#include "pch.h"

#include "FreeCamera.h"

#include "Application.h"
#include "CameraComponent.h"
#include "GameObject.h"
#include "InputModule.h"

#include <SDL_mouse.h>

bool FreeCamera::Init()
{
    camera = parent->GetComponent<CameraComponent*>();
    if (!camera) GLOG("FreeCamera: camera component not found for %s", parent->GetName().c_str())
    else camera->SetFreeCamera(true);

    return true;
}

void FreeCamera::Update(float deltaTime)
{
    if (camera == nullptr) return;

    const KeyState* keyboard     = AppEngine->GetInputModule()->GetKeyboard();
    const KeyState* mouseButtons = AppEngine->GetInputModule()->GetMouseButtons();
    const float2& mouseMotion    = AppEngine->GetInputModule()->GetMouseMotion();

    float scaleFactor            = 1.0f;
    if (keyboard[SDL_SCANCODE_LSHIFT]) scaleFactor *= 2;

    const float finalCameraSpeed       = 7.5f * scaleFactor * deltaTime;
    const float finalRotateSensitivity = 0.006f * scaleFactor;

    if (keyboard[SDL_SCANCODE_W]) camera->Translate(camera->GetCameraFront() * finalCameraSpeed);
    if (keyboard[SDL_SCANCODE_S]) camera->Translate(-camera->GetCameraFront() * finalCameraSpeed);
    if (keyboard[SDL_SCANCODE_A]) camera->Translate(-camera->GetCameraRight() * finalCameraSpeed);
    if (keyboard[SDL_SCANCODE_D]) camera->Translate(camera->GetCameraRight() * finalCameraSpeed);
    if (keyboard[SDL_SCANCODE_E]) camera->Translate(camera->GetCameraUp() * finalCameraSpeed);
    if (keyboard[SDL_SCANCODE_Q]) camera->Translate(-camera->GetCameraUp() * finalCameraSpeed);

    if (mouseButtons[SDL_BUTTON_RIGHT - 1])
    {
        const float mouseX             = mouseMotion.x;
        const float mouseY             = mouseMotion.y;
        const float deltaRotationAngle = cameraRotationAngle * finalRotateSensitivity;
        camera->Rotate(-mouseX * deltaRotationAngle, -mouseY * deltaRotationAngle);
    }
}
