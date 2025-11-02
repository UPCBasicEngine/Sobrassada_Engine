#include "pch.h"

#include "HighlightCharacter.h"

#include "CameraMovement.h"
#include "ChangeSceneScript.h"
#include "CuChulainn.h"
#include "FileSystem/FileSystem.h"
#include "GameObject.h"
#include "Globals.h"
#include "MoveGOInSpline.h"
#include "NameDisplay.h"
#include "ProjectModule.h"
#include "SavePlayerData.h"
#include "Scene.h"
#include "SceneModule.h"
#include "ScriptComponent.h"
#include "Standalone/CharacterControllerComponent.h"
#include "Standalone/Physics/CubeColliderComponent.h"
#include "Standalone/Physics/SphereColliderComponent.h"
#include "Standalone/SplineComponent.h"

HighlightCharacter::HighlightCharacter(GameObject* parent) : Script(parent)
{
    fields.emplace_back("Player", InspectorField::FieldType::InputText, &playerName);
    fields.emplace_back("Player camera pivot", InspectorField::FieldType::InputText, &playerCameraPivotName);
    fields.emplace_back("Character to highlight", InspectorField::FieldType::InputText, &characterToHighlightName);
    fields.emplace_back("Setup character on collision", InspectorField::FieldType::Bool, &setupTargetOnCollision);
    fields.emplace_back("Highlight focus", InspectorField::FieldType::InputText, &highlightFocusObjectName);
    fields.emplace_back("Use only zoom", InspectorField::FieldType::Bool, &useOnlyZoom);

    fields.emplace_back("Name display name", InspectorField::FieldType::InputText, &nameDisplayName);

    fields.emplace_back(
        "Target spline points offset", InspectorField::FieldType::Float, &secondSplinePointOffset, 0.0f, 10.0f
    );
    fields.emplace_back("Zoom multiplier", InspectorField::FieldType::Float, &zoomMultiplier, 0.1f, 50.0f);
}

bool HighlightCharacter::Init()
{
    player = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(playerName);
    if (player == nullptr)
    {
        isSetupCorrectly = false;
        GLOG("[WARNING] HighlightCharacter: No player found by the name '%s'", playerName.c_str())
        return false;
    }
    playerController = player->GetComponent<CharacterControllerComponent*>();
    if (playerController == nullptr)
    {
        isSetupCorrectly = false;
        GLOG("[WARNING] HighlightCharacter: No player controller found in player")
        return false;
    }

    playerCameraPivot = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(playerCameraPivotName);
    if (playerCameraPivot == nullptr || playerCameraPivot->GetComponent<ScriptComponent*>() == nullptr)
    {
        isSetupCorrectly = false;
        GLOG(
            "[WARNING] HighlightCharacter: No player camera with script component found by the name '%s'",
            playerCameraPivotName.c_str()
        )
        return false;
    }

    cameraMovementScript = playerCameraPivot->GetComponent<ScriptComponent*>()->GetScriptByType<CameraMovement>();
    if (cameraMovementScript == nullptr)
    {
        isSetupCorrectly = false;
        GLOG("[WARNING] HighlightCharacter: No camera movement in target `%s%", playerCameraPivotName.c_str())
        return false;
    }

    if (!setupTargetOnCollision)
    {
        characterToHighlight = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(characterToHighlightName);
        if (characterToHighlight == nullptr)
        {
            isSetupCorrectly = false;
            GLOG(
                "[WARNING] HighlightCharacter: No character to highlight found by the name '%s'",
                characterToHighlightName.c_str()
            )
            return false;
        }

        highlightFocusObject = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(highlightFocusObjectName);
        if (highlightFocusObject == nullptr) highlightFocusObject = characterToHighlight;
    }

    for (UID child : AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(parent->GetParent())->GetChildren())
    {
        GameObject* childGO = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(child);
        if (childGO != nullptr && childGO->GetComponent<ScriptComponent*>() != nullptr &&
            childGO->GetComponent<ScriptComponent*>()->GetScriptByType<MoveGOInSpline>() != nullptr)
        {
            splineMovementTarget = childGO;
            splineMovementScript = childGO->GetComponent<ScriptComponent*>()->GetScriptByType<MoveGOInSpline>();
            break;
        }
    }

    if (splineMovementScript == nullptr)
    {
        isSetupCorrectly = false;
        GLOG("[WARNING] HighlightCharacter: No spline movement script found next to this game object")
        return false;
    }

    splineComponent = parent->GetComponent<SplineComponent*>();
    if (splineComponent == nullptr)
    {
        isSetupCorrectly = false;
        GLOG("[WARNING] HighlightCharacter: No spline component found")
        return false;
    }

    splineMovementTarget->SetEnabled(false);

    GameObject* nameDisplayGO = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(nameDisplayName);
    if (nameDisplayGO == nullptr || nameDisplayGO->GetComponent<ScriptComponent*>() == nullptr)
    {
        isSetupCorrectly = false;
        GLOG("[WARNING] HighlightCharacter: No name display go found")
        return false;
    }

    nameDisplay = nameDisplayGO->GetComponent<ScriptComponent*>()->GetScriptByType<NameDisplay>();
    if (nameDisplay == nullptr)
    {
        isSetupCorrectly = false;
        GLOG("[WARNING] HighlightCharacter: Name display go doesn´t contain name display script")
        return false;
    }

    return true;
}

void HighlightCharacter::Update(float deltaTime)
{
    if (isExecuting && splineMovementScript->GetLoopCounter() > 0)
    {
        isExecuting = false;
        splineMovementTarget->SetEnabled(false);
        cameraMovementScript->ResetToDefaultTargetAndLookAhead();
        playerController->SetInputDown(true);
    }
}

void HighlightCharacter::OnDestroy()
{
    player               = nullptr;
    playerController     = nullptr;
    cameraMovementScript = nullptr;
    characterToHighlight = nullptr;
    highlightFocusObject = nullptr;
    Script::OnDestroy();
}

void HighlightCharacter::OnCollisionEnter(GameObject* otherObject, const float3 collisionNormal, ColliderLayer layer)
{
    if (!neverExecuted || otherObject != player || !isSetupCorrectly) return;

    if (parent->GetComponent<SphereColliderComponent*>() != nullptr &&
        !parent->GetComponent<CubeColliderComponent*>()->GetEnabled())
    {
        parent->GetComponent<SphereColliderComponent*>()->SetEnabled(false);
        parent->GetComponent<CubeColliderComponent*>()->SetEnabled(true);
    }
    else
    {
        if (setupTargetOnCollision)
        {
            characterToHighlight =
                AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(characterToHighlightName);
            if (characterToHighlight == nullptr)
            {
                isSetupCorrectly = false;
                GLOG(
                    "[WARNING] HighlightCharacter: No character to highlight found by the name '%s'",
                    characterToHighlightName.c_str()
                )
                neverExecuted = false;
                return;
            }

            highlightFocusObject =
                AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(highlightFocusObjectName);
            if (highlightFocusObject == nullptr) highlightFocusObject = characterToHighlight;
        }

        playerController->SetInputDown(false);
        if (player->GetComponent<ScriptComponent*>()->GetScriptByType<CuChulainn>())
            player->GetComponent<ScriptComponent*>()->GetScriptByType<CuChulainn>()->ResetState();

        const float3 highlightVector =
            (highlightFocusObject->GetGlobalTransform().TranslatePart() - parent->GetGlobalTransform().TranslatePart())
                .Normalized();
        Quat cameraOrientation = Quat(
            AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName("Camera")->GetGlobalTransform().RotatePart()
        );
        const float3 zoomVector = cameraOrientation.Transform(float3(0, 0, -1)).Normalized();

        if (useOnlyZoom)
        {
            splineComponent->SetPointWorld(
                0, highlightFocusObject->GetGlobalTransform().TranslatePart() + zoomMultiplier * zoomVector
            );
            splineComponent->SetPointWorld(
                1, highlightFocusObject->GetGlobalTransform().TranslatePart() + .6f * zoomMultiplier * zoomVector
            );
            splineComponent->SetPointWorld(
                2, highlightFocusObject->GetGlobalTransform().TranslatePart() + .3f * zoomMultiplier * zoomVector
            );
            splineComponent->SetPointWorld(3, characterToHighlight->GetGlobalTransform().TranslatePart());
        }
        else
        {
            splineComponent->SetPointWorld(0, playerCameraPivot->GetGlobalTransform().TranslatePart());

            splineComponent->SetPointWorld(
                1, highlightFocusObject->GetGlobalTransform().TranslatePart() -
                       highlightVector * secondSplinePointOffset + secondSplinePointOffset / 2.f * zoomVector
            );
            splineComponent->SetPointWorld(
                2, highlightFocusObject->GetGlobalTransform().TranslatePart() + secondSplinePointOffset * zoomVector -
                       secondSplinePointOffset / 2.f * highlightVector
            );
            splineComponent->SetPointWorld(
                3, highlightFocusObject->GetGlobalTransform().TranslatePart() + zoomMultiplier * zoomVector
            );
        }

        splineMovementTarget->SetEnabled(true);
        cameraMovementScript->InitAlternativeTargetAndLookAhead(splineMovementTarget, 0.f);

        nameDisplay->ShowWithDelay();

        characterToHighlight->GetComponent<ScriptComponent*>()->GetScriptByType<Character>()->PlayHighlightSequence();
        isExecuting   = true;
        neverExecuted = false;
    }
}