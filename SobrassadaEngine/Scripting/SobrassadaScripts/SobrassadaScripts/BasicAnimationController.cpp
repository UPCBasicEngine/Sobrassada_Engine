
#include "pch.h"

#include "BasicAnimationController.h"
#include "Standalone/AnimationComponent.h"
#include "GameObject.h"

BasicAnimationController::BasicAnimationController(GameObject* parent) : Script(parent) {}

bool BasicAnimationController::Init()
{
    animComponent = parent->GetComponent<AnimationComponent*>();
    if (!animComponent)
        GLOG("Animation component not found in basic animation controller %s", parent->GetName().c_str())
    else animComponent->OnPlay(false);

    return true;
}
