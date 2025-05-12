#include "CubeColliderComponent.h"

#include "Application.h"
#include "GameObject.h"
#include "PhysicsModule.h"
#include "SceneModule.h"
#include "ScriptComponent.h"

#include "ImGui.h"
#include "rapidjson/document.h"

CubeColliderComponent::CubeColliderComponent(UID uid, GameObject* parent)
    : Component(uid, parent, "Cube Collider", COMPONENT_CUBE_COLLIDER)
{

    parent->UpdateMobilityHierarchy(MobilitySettings::DYNAMIC);

    CalculateCollider();

    onCollissionCallback = CollisionDelegate(
        std::bind(&CubeColliderComponent::OnCollision, this, std::placeholders::_1, std::placeholders::_2)
    );
    userPointer = BulletUserPointer(this, &onCollissionCallback, generateCallback);
    App->GetPhysicsModule()->CreateCubeRigidBody(this);
}

CubeColliderComponent::CubeColliderComponent(const rapidjson::Value& initialState, GameObject* parent)
    : Component(initialState, parent)
{
    if (initialState.HasMember("FreezeRotation")) freezeRotation = initialState["FreezeRotation"].GetBool();
    if (initialState.HasMember("Mass")) mass = initialState["Mass"].GetFloat();
    if (initialState.HasMember("ColliderType")) colliderType = ColliderType(initialState["ColliderType"].GetInt());
    if (initialState.HasMember("ColliderLayer")) layer = ColliderLayer(initialState["ColliderLayer"].GetInt());
    if (initialState.HasMember("GenerateCallback")) generateCallback = initialState["GenerateCallback"].GetBool();
    if (initialState.HasMember("FitToSize")) fitToSize = initialState["FitToSize"].GetBool();

    if (initialState.HasMember("CenterOffset"))
    {
        const rapidjson::Value& dataArray = initialState["CenterOffset"];
        centerOffset                      = {dataArray[0].GetFloat(), dataArray[1].GetFloat(), dataArray[2].GetFloat()};
    }

    if (initialState.HasMember("CenterRotation"))
    {
        const rapidjson::Value& dataArray = initialState["CenterRotation"];
        centerRotation                    = {dataArray[0].GetFloat(), dataArray[1].GetFloat(), dataArray[2].GetFloat()};
    }

    if (initialState.HasMember("Size"))
    {
        const rapidjson::Value& dataArray = initialState["Size"];
        size                              = {dataArray[0].GetFloat(), dataArray[1].GetFloat(), dataArray[2].GetFloat()};
    }

    if (colliderType == ColliderType::STATIC && !parent->IsStatic())
        parent->UpdateMobilityHierarchy(MobilitySettings::STATIC);
    else if (!(colliderType == ColliderType::STATIC) && parent->IsStatic())
        parent->UpdateMobilityHierarchy(MobilitySettings::DYNAMIC);

    onCollissionCallback = CollisionDelegate(
        std::bind(&CubeColliderComponent::OnCollision, this, std::placeholders::_1, std::placeholders::_2)
    );
    userPointer = BulletUserPointer(this, &onCollissionCallback, generateCallback);
    App->GetPhysicsModule()->CreateCubeRigidBody(this);
}

CubeColliderComponent::~CubeColliderComponent()
{
    DeleteRigidBody();
}

void CubeColliderComponent::Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const
{
    Component::Save(targetState, allocator);

    targetState.AddMember("FreezeRotation", freezeRotation, allocator);
    targetState.AddMember("Mass", mass, allocator);
    targetState.AddMember("ColliderType", (int)colliderType, allocator);
    targetState.AddMember("ColliderLayer", (int)layer, allocator);
    targetState.AddMember("GenerateCallback", generateCallback, allocator);
    targetState.AddMember("FitToSize", fitToSize, allocator);

    // CENTER OFFSET
    rapidjson::Value centerOffsetSave(rapidjson::kArrayType);
    centerOffsetSave.PushBack(centerOffset.x, allocator)
        .PushBack(centerOffset.y, allocator)
        .PushBack(centerOffset.z, allocator);
    targetState.AddMember("CenterOffset", centerOffsetSave, allocator);

    // CENTER ROTATION
    rapidjson::Value centerRotationSave(rapidjson::kArrayType);
    centerRotationSave.PushBack(centerRotation.x, allocator)
        .PushBack(centerRotation.y, allocator)
        .PushBack(centerRotation.z, allocator);
    targetState.AddMember("CenterRotation", centerRotationSave, allocator);

    // BOX SIZE
    rapidjson::Value sizeSave(rapidjson::kArrayType);
    sizeSave.PushBack(size.x, allocator).PushBack(size.y, allocator).PushBack(size.z, allocator);
    targetState.AddMember("Size", sizeSave, allocator);
}

void CubeColliderComponent::Clone(const Component* other)
{
    if (other->GetType() == COMPONENT_CUBE_COLLIDER)
    {
        const CubeColliderComponent* cube = static_cast<const CubeColliderComponent*>(other);

        generateCallback                  = cube->generateCallback;
        freezeRotation                    = cube->freezeRotation;
        fitToSize                         = cube->fitToSize;
        mass                              = cube->mass;
        centerOffset                      = cube->centerOffset;
        centerRotation                    = cube->centerRotation;
        size                              = cube->size;
        colliderType                      = cube->colliderType;
        layer                             = cube->layer;

        if (rigidBody) App->GetPhysicsModule()->UpdateCubeRigidBody(this);
        else App->GetPhysicsModule()->CreateCubeRigidBody(this);
    }
}

void CubeColliderComponent::RenderEditorInspector()
{
    Component::RenderEditorInspector();

    if (enabled)
    {
        ImGui::SeparatorText("Capsule Collider Component");
        if (ImGui::BeginCombo("Collider type", ColliderTypeStrings[(int)colliderType]))
        {
            const int colliderStringSize = sizeof(ColliderTypeStrings) / sizeof(char*);
            for (int i = 0; i < colliderStringSize; ++i)
            {
                if (ImGui::Selectable(ColliderTypeStrings[i]))
                {
                    colliderType = ColliderType(i);
                    if (colliderType == ColliderType::STATIC)
                    {
                        parent->UpdateMobilityHierarchy(MobilitySettings::STATIC);
                        mass = 0.f;
                    }

                    else
                    {
                        parent->UpdateMobilityHierarchy(MobilitySettings::DYNAMIC);
                        mass = 1.f;
                    }

                    App->GetPhysicsModule()->UpdateCubeRigidBody(this);
                }
            }
            ImGui::EndCombo();
        }

        ImGui::BeginDisabled(colliderType == ColliderType::STATIC);
        if (ImGui::InputFloat("Mass", &mass))
        {
            App->GetPhysicsModule()->UpdateCubeRigidBody(this);
        }
        ImGui::EndDisabled();

        if (ImGui::InputFloat3("Center offset", &centerOffset[0])) App->GetPhysicsModule()->UpdateCubeRigidBody(this);

        if (ImGui::InputFloat3("Size", &size[0])) App->GetPhysicsModule()->UpdateCubeRigidBody(this);

        if (ImGui::Checkbox("Freeze rotation", &freezeRotation)) App->GetPhysicsModule()->UpdateCubeRigidBody(this);

        if (ImGui::InputFloat3("Center rotation", &centerRotation[0]))
            App->GetPhysicsModule()->UpdateCubeRigidBody(this);

        // COLLIDER LAYER SETTINGS
        if (ImGui::BeginCombo("Layer options", ColliderLayerStrings[(int)layer]))
        {
            const int colliderStringSize = sizeof(ColliderLayerStrings) / sizeof(char*);
            for (int i = 0; i < colliderStringSize; ++i)
            {
                if (ImGui::Selectable(ColliderLayerStrings[i]))
                {
                    layer = ColliderLayer(i);
                    App->GetPhysicsModule()->UpdateCubeRigidBody(this);
                }
            }
            ImGui::EndCombo();
        }

        if (ImGui::Checkbox("Fit to size", &fitToSize))
        {
            CalculateCollider();
            App->GetPhysicsModule()->UpdateCubeRigidBody(this);
        }

        if (ImGui::Checkbox("Generate Callbacks", &generateCallback))
        {
            userPointer = BulletUserPointer(this, &onCollissionCallback, generateCallback);
        }
    }
}

void CubeColliderComponent::Update(float deltaTime)
{
    if (!IsEffectivelyEnabled()) return;
}

void CubeColliderComponent::Render(float deltaTime)
{
    if (!IsEffectivelyEnabled()) return;
}

void CubeColliderComponent::RenderDebug(float deltaTime)
{
}

void CubeColliderComponent::ParentUpdated()
{
    if (App->GetSceneModule()->GetInPlayMode()) return;

    if (fitToSize) CalculateCollider();

    if (parent->IsStatic() && colliderType != ColliderType::STATIC)
    {
        mass         = 0.f;
        colliderType = ColliderType::STATIC;
    }
    else if (!parent->IsStatic() && colliderType == ColliderType::STATIC)
    {
        mass         = 1.f;
        colliderType = ColliderType::DYNAMIC;
    }

    App->GetPhysicsModule()->UpdateCubeRigidBody(this);
}

void CubeColliderComponent::OnCollision(GameObject* otherObject, float3 collisionNormal)
{
    if (!enabled || !otherObject->IsEnabled()) return;

    auto script = parent->GetComponent<ScriptComponent*>();
    
    if (script) script->OnCollision(otherObject, collisionNormal);
}

void CubeColliderComponent::DeleteRigidBody()
{
    App->GetPhysicsModule()->DeleteCubeRigidBody(this);
}

void CubeColliderComponent::CalculateCollider()
{
    AABB heriachyAABB               = parent->GetHierarchyAABB();

    const float4x4& globalTransform = parent->GetGlobalTransform();

    if (heriachyAABB.IsFinite() && !heriachyAABB.IsDegenerate())
    {
        size         = heriachyAABB.HalfSize();
        centerOffset = heriachyAABB.CenterPoint() - globalTransform.TranslatePart();
    }
}
