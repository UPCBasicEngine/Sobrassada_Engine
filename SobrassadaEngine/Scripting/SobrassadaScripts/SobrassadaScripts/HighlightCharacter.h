#pragma once
#include "Script.h"

class NameDisplay;
class MoveGOInSpline;
class SplineComponent;
class CameraMovement;
class CharacterControllerComponent;

class HighlightCharacter : public Script
{
  public:
    HighlightCharacter(GameObject* parent);

    bool Init() override;
    void Update(float deltaTime) override;
    void OnDestroy() override;
    void OnCollisionEnter(GameObject* otherObject, const float3 collisionNormal, ColliderLayer layer) override;

  private:
    std::string playerName            = "walk";
    std::string playerCameraPivotName = "Camera Pivot";
    std::string characterToHighlightName;
    std::string highlightFocusObjectName;

    std::string nameDisplayName;

    float secondSplinePointOffset                  = 0.85f;
    float zoomMultiplier                           = 30.0f;

    bool hidePlayerWhileZooming                    = true;
    bool useOnlyZoom                               = false;

    bool isSetupCorrectly                          = true;
    bool setupTargetOnCollision                    = false;
    bool isExecuting                               = false;
    bool neverExecuted                             = true;

    GameObject* player                             = nullptr;
    CharacterControllerComponent* playerController = nullptr;
    GameObject* playerCameraPivot                  = nullptr;
    CameraMovement* cameraMovementScript           = nullptr;
    GameObject* characterToHighlight               = nullptr;
    GameObject* highlightFocusObject               = nullptr;
    SplineComponent* splineComponent               = nullptr;
    GameObject* splineMovementTarget               = nullptr;
    MoveGOInSpline* splineMovementScript           = nullptr;

    NameDisplay* nameDisplay                       = nullptr;
};