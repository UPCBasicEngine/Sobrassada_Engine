#include "pch.h"
#include "MoveGOInSpline.h"

#include "Application.h"
#include "GameObject.h"
#include "SceneModule.h"
#include "Standalone/SplineComponent.h"


MoveGOInSpline::MoveGOInSpline(GameObject* parent) : Script(parent)
{
    fields.push_back({"Speed (0-1/s)", InspectorField::FieldType::Float, &speed, 0.0f, 10.0f});
    fields.push_back({"PingPong effect", InspectorField::FieldType::Bool, &pingPong});
}

bool MoveGOInSpline::Init()
{
    spline = FindSpline();
    
    if (!spline) GLOG("[MoveGOInSpline] GameObject '%s' did not found spline.", parent->GetName().c_str());
    
    return true;
}

void MoveGOInSpline::Update(float deltaTime)
{
    if (!spline)
    {
        spline = FindSpline();
        if (!spline) return;
    }
    if (speed <= 0.0f) return;

    float delta  = speed * deltaTime * (goingForward ? 1.0f : -1.0f);
    t += delta;

    loop  = spline->IsLoop();
    if (loop)  //Closed spline
    {
        if (t > 1.f) t -= 1.f;
        else if (t < 0.f) t += 1.f;
    }
    else    //Open Spline
    {
        if (pingPong)
        {
            if (t > 1.f)
            {
                t            = 1.f;
                goingForward = false;
            }
            else if (t < 0)
            {
                t            = 0.f;
                goingForward = true;
            }
        }
        else
        {
            if (t > 1.f) t -= 1.f;
            else if (t < 0.f) t += 1.f;
        }
    }

    float3 worldPos = spline->GetWorldPositionInSpine(t);

    float4x4 local  = parent->GetLocalTransform();
    local.SetTranslatePart(parent->GetParentGlobalTransform().Inverted().TransformPos(worldPos));

    parent->SetLocalTransform(local);

}

SplineComponent* MoveGOInSpline::FindSpline()
{
    UID parentUID = parent->GetParent();
    GameObject* parent = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(parentUID);

    if (!parent) return nullptr;

    for (UID childUID : parent->GetChildren())
    {
        if (childUID == parent->GetUID()) continue;
        GameObject* child = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(childUID);

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


