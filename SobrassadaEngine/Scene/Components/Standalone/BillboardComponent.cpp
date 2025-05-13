#include "BillboardComponent.h"

#include "Application.h"
#include "CameraComponent.h"
#include "CameraModule.h"
#include "GameObject.h"
#include "SceneModule.h"

BillboardComponent::BillboardComponent(UID uid, GameObject* parent)
    : Component(uid, parent, "Billboard", COMPONENT_BILLBOARD)
{
}

BillboardComponent::BillboardComponent(const rapidjson::Value& initialState, GameObject* parent)
    : Component(initialState, parent)
{
}

BillboardComponent::~BillboardComponent()
{
}

void BillboardComponent::Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const
{
    Component::Save(targetState, allocator);
}

void BillboardComponent::Clone(const Component* other)
{
    if (other->GetType() == ComponentType::COMPONENT_BILLBOARD)
    {
        const BillboardComponent* otherBillboard = static_cast<const BillboardComponent*>(other);
        enabled                                  = otherBillboard->enabled;
        wasEnabled                               = otherBillboard->wasEnabled;
    }
}

void BillboardComponent::Update(float deltaTime)
{
    CameraComponent* activeCamera = App->GetSceneModule()->GetScene()->GetMainCamera();
    if (activeCamera)
    {
    }
    else
    {
        const Frustum& editorCamera = App->GetCameraModule()->GetCamera();
        int x                       = 0;
    }
}

void BillboardComponent::Render(float deltaTime)
{
}

void BillboardComponent::RenderDebug(float deltaTime)
{
}
