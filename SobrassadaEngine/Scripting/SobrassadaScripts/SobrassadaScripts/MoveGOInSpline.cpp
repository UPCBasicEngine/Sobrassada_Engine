#include "pch.h"
#include "MoveGOInSpline.h"

#include "Application.h"
#include "GameObject.h"
#include "SceneModule.h"
#include "Standalone/SplineComponent.h"


MoveGOInSpline::MoveGOInSpline(GameObject* parent) : Script(parent)
{
    fields.push_back({"Speed Factor", InspectorField::FieldType::Float, &speed, 0.0f, 10.0f});
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

    float segSpeed = spline->EvaluateSpeed(t) * speed;

    const int ptCount  = static_cast<int>(spline->GetNumPoints());
    const bool isLoop  = spline->IsLoop();
    const int segCount = isLoop ? ptCount : ptCount - 1;

    float segFloat     = t * segCount;
    int idxStart           = static_cast<int>(floorf(segFloat));
    
    if (isLoop) idxStart %= ptCount;

    int idxEnd    = isLoop ? (idxStart + 1) % ptCount : (std::min)(idxStart + 1, ptCount - 1);
    
    float3 pStart = spline->GetPointWorld(idxStart);
    float3 pEnd   = spline->GetPointWorld(idxEnd);
    float segLen  = (pEnd - pStart).Length();

    if (segLen < 0.0001f) segLen = 0.0001f;
    
    float deltaT = (segSpeed * deltaTime) / (segLen * segCount);

    if (pingPong && !isLoop)
    {
        t += goingForward ? deltaT : -deltaT;

        if (t > 1.f)
        {
            t            = 2.0f - t;
            goingForward = false;
        }
        else if (t < 0)
        {
            t            = -t;
            goingForward = true;
            loopCounter++;
        }
    }
    else
    {
        t += deltaT;
        if (t > 1.f)
        {
            t -= 1.f;
            loopCounter++;
        }
        else if (t < 0.f)
        {
            t += 1.f;
            loopCounter++;
        }
    }

    float3 worldPos;
    Quat worldRot;
    spline->EvaluateTransform(t, worldPos, worldRot);
    
    const float4x4& parentGlob = parent->GetParentGlobalTransform();

    float4x4 worldM            = float4x4::FromTRS(worldPos, worldRot.ToFloat4x4(), float3::one);
    
    float4x4 localM            = parentGlob.Inverted() * worldM;
    parent->SetLocalTransform(localM);

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
            splineIdGO = childUID;
            return spline;
        }
    }
    
    return nullptr;
}


