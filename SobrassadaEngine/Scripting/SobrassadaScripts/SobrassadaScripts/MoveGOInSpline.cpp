#include "pch.h"
#include "MoveGOInSpline.h"

#include "Application.h"
#include "GameObject.h"
#include "SceneModule.h"
#include "Standalone/SplineComponent.h"


MoveGOInSpline::MoveGOInSpline(GameObject* parent) : Script(parent)
{
}

bool MoveGOInSpline::Init()
{
    spline = FindSpline();
    
    if (!spline) GLOG("[MoveGOInSpline] GameObject '%s' did not found spline.", parent->GetName().c_str());
    
    return true;
}

SplineComponent* MoveGOInSpline::FindSpline()
{
    UID parentUID = parent->GetParent();
    GameObject* parent = App->GetSceneModule()->GetScene()->GetGameObjectByUID(parentUID);

    if (!parent) return nullptr;

    for (UID childUID : parent->GetChildren())
    {
        if (childUID == parent->GetUID()) continue;
        GameObject* child = App->GetSceneModule()->GetScene()->GetGameObjectByUID(childUID);

        if (!child) continue;

        SplineComponent* spline = child->GetComponent<SplineComponent*>();

        if (spline)
        {
            splineGO = childUID;
            return spline;
        }
    }
    
    return nullptr;
}


