#include "pch.h"

#include "TileFloatScript.h"

#include "CuChulainn.h"
#include "EditorUIModule.h"
#include "GameObject.h"
#include "Standalone/CharacterControllerComponent.h"
#include "Wwise_IDs.h"
#include "imgui_curve_editor.h"

#include "Math/float4x4.h"
#include "Standalone/Audio/AudioSourceComponent.h"

#include <Math/MathFunc.h>

TileFloatScript::TileFloatScript(GameObject* parent) : Script(parent)
{
    fields.emplace_back("Speed", InspectorField::FieldType::Float, &speed, 1, 10);
    fields.emplace_back("MinDistanceToPlayer", InspectorField::FieldType::Float, &minDistanceToPlayer, -100.0f, 100.0f);
    fields.emplace_back("Starting Position", InspectorField::FieldType::Vec3, &startPosition, -100.0, 100.0f);
    fields.emplace_back("Starting Rotation", InspectorField::FieldType::Vec3, &startRotation, -100.0, 100.0f);
    fields.emplace_back("Starting Scale", InspectorField::FieldType::Vec3, &startScale, -100.0, 100.0f);
    fields.emplace_back(
        "Set Start Transform",
        [this](Script* self)
        {
            const float4x4& currentTransform = this->parent->GetLocalTransform();

            this->startPosition              = currentTransform.TranslatePart();
            this->startRotation              = currentTransform.RotatePart().ToEulerXYZ();
            this->startScale                 = currentTransform.GetScale();
            Quat rotQuat                     = Quat(currentTransform.RotatePart());
        }
    );
}

bool TileFloatScript::Init()
{

    if (parent->GetChildren().empty())
    {
        isSetupCorrectly = false;
        GLOG("[ERROR] No movable tile as child found")
        return false;
    }

    tileToMove = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(parent->GetChildren()[0]);
    if (tileToMove == nullptr)
    {
        isSetupCorrectly = false;
        GLOG("[ERROR] Tile game object not found")
        return false;
    }

    // get final (correct) position, and move the tile to start (rotated, moved and scaled) position
    const float4x4& originalTransform = tileToMove->GetLocalTransform();
    finalPosition                     = originalTransform.TranslatePart();
    finalRotation                     = Quat(originalTransform.RotatePart());
    finalScale                        = originalTransform.GetScale();

    startQuat                         = Quat::FromEulerXYZ(startRotation.x, startRotation.y, startRotation.z);

    const float4x4 startTransform     = float4x4::FromTRS(startPosition, startQuat, startScale);
    currentRotationQuat               = startQuat;

    tileToMove->SetLocalTransform(startTransform);

    // Audio
    audioComp = tileToMove->GetComponent<AudioSourceComponent*>();

    // GLOG("Initiating TileFloatScript");
    return true;
}

void TileFloatScript::Update(float deltaTime)
{
    if (!isSetupCorrectly || isFinished || !character) return;

    if (!isActive)
    {
        const float distanceSq =
            character->GetLastPosition().DistanceSq(finalPosition + parent->GetGlobalTransform().TranslatePart());

        isActive = distanceSq < minDistanceToPlayer * minDistanceToPlayer;

        if (isActive && audioComp != nullptr) audioComp->EmitEvent(AK::EVENTS::PLAY_SFX_TILES);
    }
    else
    {
        risingCounter     += deltaTime / (10.0f / speed);
        const float alpha  = risingCounter < .5f ? 4 * Pow(risingCounter, 3) : 1 - Pow(-2 * risingCounter + 2, 3) / 2;

        // Ensure shortest path
        if (QuaternionDot(currentRotationQuat, finalRotation) < 0.0f)
        {
            finalRotation = Quat(-finalRotation.x, -finalRotation.y, -finalRotation.z, -finalRotation.w);
        }

        // Snap to final transform if very close
        if (risingCounter >= 1.0f)
        {
            currentRotationQuat       = finalRotation;
            float4x4 snappedTransform = float4x4::FromTRS(finalPosition, currentRotationQuat, finalScale);
            tileToMove->SetLocalTransform(snappedTransform);
            isFinished = true;
            return;
        }

        const float3 newT           = Lerp(tileToMove->GetPosition(), finalPosition, alpha);
        const float3 newS           = Lerp(tileToMove->GetScale(), finalScale, alpha);
        currentRotationQuat         = Quat::Slerp(currentRotationQuat, finalRotation, alpha).Normalized();

        const float4x4 newTransform = float4x4::FromTRS(newT, currentRotationQuat, newS);
        tileToMove->SetLocalTransform(newTransform);
    }
}
