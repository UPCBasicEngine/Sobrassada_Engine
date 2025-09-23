#include "pch.h"

#include "HighlightCharacter.h"

#include "CameraMovement.h"
#include "ChangeSceneScript.h"
#include "CuChulainn.h"
#include "FileSystem/FileSystem.h"
#include "GameObject.h"
#include "Globals.h"
#include "MoveGOInSpline.h"
#include "ProjectModule.h"
#include "SavePlayerData.h"
#include "Scene.h"
#include "SceneModule.h"
#include "ScriptComponent.h"
#include "Standalone/CharacterControllerComponent.h"
#include "Standalone/Physics/CubeColliderComponent.h"
#include "Standalone/SplineComponent.h"

HighlightCharacter::HighlightCharacter(GameObject* parent) : Script(parent)
{
    fields.emplace_back("Player", InspectorField::FieldType::InputText, &playerName);
    fields.emplace_back("Player camera pivot", InspectorField::FieldType::InputText, &playerCameraPivotName);
    fields.emplace_back("Character to highlight", InspectorField::FieldType::InputText, &characterToHighlightName);

    fields.emplace_back(
        "Target spline points offset", InspectorField::FieldType::Float, &secondSplinePointOffset, 0.1f, 2.0f
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

    return true;
}

void HighlightCharacter::Update(float deltaTime)
{
    if (isExecuting && splineMovementScript->GetLoopCounter() > 0)
    {
        isExecuting = false;
        splineMovementTarget->SetEnabled(false);
        cameraMovementScript->ResetToDefaultTarget();
        AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName("CH_MC_Chu_V02")->SetEnabled(true);
        AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName("WP_Spear_Cu_Chu")->SetEnabled(true);
        playerController->SetInputDown(true);
    }
}

void HighlightCharacter::OnDestroy()
{
    player               = nullptr;
    playerController     = nullptr;
    cameraMovementScript = nullptr;
    characterToHighlight = nullptr;
    Script::OnDestroy();
}

void HighlightCharacter::OnCollisionEnter(GameObject* otherObject, const float3 collisionNormal, ColliderLayer layer)
{
    if (!neverExecuted || otherObject != player) return;

    playerController->SetInputDown(false);
    AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName("CH_MC_Chu_V02")->SetEnabled(false);
    AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName("WP_Spear_Cu_Chu")->SetEnabled(false);
    
    splineComponent->SetPointWorld(
        0, playerCameraPivot->GetGlobalTransform().TranslatePart() - parent->GetGlobalTransform().TranslatePart()
    );
    splineComponent->SetPointWorld(
        1, 0.85f * (characterToHighlight->GetGlobalTransform().TranslatePart() -
                    parent->GetGlobalTransform().TranslatePart())
    );
    Quat cameraOrientation =
        Quat(AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName("Camera")->GetGlobalTransform().RotatePart());
    splineComponent->SetPointWorld(
        2, 0.85f * (characterToHighlight->GetGlobalTransform().TranslatePart() -
                    parent->GetGlobalTransform().TranslatePart()) +
               (30 * cameraOrientation.Transform(float3(0, 0, -1)))
    );
    splineMovementTarget->SetEnabled(true);
    cameraMovementScript->InitAlternativeTarget(splineMovementTarget);

    characterToHighlight->GetComponent<ScriptComponent*>()->GetScriptByType<Character>()->PlayHighlightSequence();
    isExecuting   = true;
    neverExecuted = false;
}