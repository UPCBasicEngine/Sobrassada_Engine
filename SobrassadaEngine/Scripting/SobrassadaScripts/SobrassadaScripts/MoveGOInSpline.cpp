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

    const float deltaT  = speed * deltaTime;
    

    isLoop  = spline->IsLoop();
    if (pingPong && !isLoop)
    {
        t += goingForward ? deltaT : -deltaT;

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
        t += deltaT;
        if (t > 1.f) t -= 1.f;
        else if (t < 0.f) t += 1.f;
    }

    float3 localPos;
    Quat localRot;
    spline->EvaluateTransform(t, localPos, localRot);

    GameObject* splineGO = spline->GetParent();
    const float4x4& splineGlob = splineGO->GetGlobalTransform();

    float3 worldPos            = splineGlob.TransformPos(localPos);
    Quat worldRot              = Quat(splineGlob) * localRot;
    
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


