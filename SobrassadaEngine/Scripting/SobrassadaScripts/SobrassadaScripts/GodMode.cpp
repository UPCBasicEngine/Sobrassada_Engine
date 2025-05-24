#include "pch.h"

#include "Application.h"
#include "CameraComponent.h"
#include "Components/Standalone/CharacterControllerComponent.h"
#include "GameObject.h"
#include "GodMode.h"
#include "InputModule.h"

#include <SDL_mouse.h>

GodMode::GodMode(GameObject* parent) : Script(parent)
{
    fields.push_back({"Camera Name", InspectorField::FieldType::InputText, &cameraName});
}

bool GodMode::Init()
{
    characterController = parent->GetComponent<CharacterControllerComponent*>();
    if (!characterController)
        GLOG("[WARNING] GodMode character controller component not found for %s", parent->GetName().c_str());

    GameObject* cameraGameObject = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(cameraName);
    if (cameraGameObject)
    {
        godCamera = cameraGameObject->GetComponent<CameraComponent*>();
        if (!godCamera) GLOG("[WARNING] GodMode camera component not found for %s", parent->GetName().c_str());
    }
    else GLOG("[WARNING] No camera found by the name: %s", cameraName.c_str())

    return true;
}

void GodMode::Update(float deltaTime)
{
    if (!characterController || !godCamera) return;

    const KeyState* keyboard = AppEngine->GetInputModule()->GetKeyboard();

    if (freeCamera)
    {
        const KeyState* mouseButtons = AppEngine->GetInputModule()->GetMouseButtons();
        const float2& mouseMotion    = AppEngine->GetInputModule()->GetMouseMotion();

        float scaleFactor            = 1.0f;
        if (keyboard[SDL_SCANCODE_LSHIFT]) scaleFactor *= 2;

        const float finalCameraSpeed       = 7.5f * scaleFactor * deltaTime;
        const float finalRotateSensitivity = 0.006f * scaleFactor;

        if (keyboard[SDL_SCANCODE_W]) godCamera->Translate(godCamera->GetCameraFront() * finalCameraSpeed);
        if (keyboard[SDL_SCANCODE_S]) godCamera->Translate(-godCamera->GetCameraFront() * finalCameraSpeed);
        if (keyboard[SDL_SCANCODE_A]) godCamera->Translate(-godCamera->GetCameraRight() * finalCameraSpeed);
        if (keyboard[SDL_SCANCODE_D]) godCamera->Translate(godCamera->GetCameraRight() * finalCameraSpeed);
        if (keyboard[SDL_SCANCODE_E]) godCamera->Translate(godCamera->GetCameraUp() * finalCameraSpeed);
        if (keyboard[SDL_SCANCODE_Q]) godCamera->Translate(-godCamera->GetCameraUp() * finalCameraSpeed);

        if (mouseButtons[SDL_BUTTON_RIGHT - 1])
        {
            const float mouseX             = mouseMotion.x;
            const float mouseY             = mouseMotion.y;
            const float deltaRotationAngle = cameraRotationAngle * finalRotateSensitivity;
            godCamera->Rotate(-mouseX * deltaRotationAngle, -mouseY * deltaRotationAngle);
        }

        if (keyboard[SDL_SCANCODE_O] == KEY_DOWN)
        {
            freeCamera = false;
            godCamera->SetFreeCamera(false);
            characterController->SetInputDown(true);
        }
    }
    if (keyboard[SDL_SCANCODE_P] == KEY_DOWN)
    {
        characterController->SetInputDown(false);
        godCamera->SetFreeCamera(true);
        freeCamera = true;
    }
}