#pragma once

#include "Script.h"

class GameObject;
class ImageComponent;
class SphereColliderComponent;

class SpawnUI : public Script
{
  public:
    SpawnUI(GameObject* parent);
    bool Init() override;
    void Update(float deltaTime) override;
    void OnCollisionEnter(GameObject* otherObject, const float3 collisionNormal, ColliderLayer layer) override;
    void OnCollisionExit(GameObject* otherObject, ColliderLayer layer) override;

  private:
    void ShowUI();

  private:
    SphereColliderComponent* trigger       = nullptr;
    ImageComponent* imageUI                = nullptr;
    ImageComponent* xboxAlternativeImageUI = nullptr;
    ImageComponent* psAlternativeImageUI   = nullptr;
    bool triggerOnce                       = false;
    float showDelay                        = 0.0f;
    float hideAfterSeconds                 = 0.f;
    std::string objectUIName;
    std::string xboxAlternativeObjectUIName;
    std::string psAlternativeObjectUIName;

    bool unlockAbility      = false;
    std::string nameAbility = "";

    float timer             = 0.f;
    bool delayedShowing     = false;
    bool updating;

    GameObject* cachedCollisionObject;
};
