#include "pch.h"

#include "AttackVfx.h"

#include "Application.h"
#include "CuChulainn.h"
#include "GameObject.h"
#include "Math/Quat.h"
#include "ResourcesModule.h"
#include "Scene.h"
#include "SceneModule.h"
#include "Standalone/CharacterControllerComponent.h"

AttackVfx::AttackVfx(GameObject* parent) : Script(parent)
{
    fields.push_back({"Camera Object Name", InspectorField::FieldType::InputText, &cameraName});
    fields.push_back({"Full Billboard", InspectorField::FieldType::Bool, &fullBillboard});
}

bool AttackVfx::Init()
{
    camera = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(cameraName);
    if (!camera) GLOG("[AttackVfx - Warning] No camera gameObject found by name: %s", cameraName.c_str());

    return true;
}

void AttackVfx::Update(float deltaTime)
{
    // Face camera
    float4x4 planeTransform = parent->GetGlobalTransform();

    float3 camDir           = (camera->GetGlobalTransform().TranslatePart() - planeTransform.TranslatePart());
    if (!fullBillboard) camDir.y = 0.0f;
    camDir.Normalize();

    float3 refUp = float3::unitY;
    if (abs(Dot(camDir, refUp)) > 0.99f) // nearly parallel
        refUp = float3::unitX;           // fallback

    float3 camRight   = camera->GetGlobalTransform().Col3(0).Normalized();
    float3 camUp      = camera->GetGlobalTransform().Col3(1).Normalized();

    float3 right      = Cross(refUp, camDir).Normalized();
    float3 up         = Cross(camDir, right);

    float3x3 rotMat   = fullBillboard ? float3x3(camRight, camUp, camDir) : float3x3(right, up, camDir);
    Quat billboardRot = Quat(rotMat);

    planeTransform =
        float4x4::FromTRS(character->GetParent()->GetPosition() + float3::unitY, billboardRot, float3::one);

    Quat adjustRot = Quat::RotateAxisAngle(float3::unitX, -PI / 2.0f);
    planeTransform = planeTransform * float4x4(adjustRot, float3(0, 0, 0));

    // TODO: Rotate according to player dir

    parent->SetLocalTransform(planeTransform);
}
