#include "Transform2DComponent.h"

#include "Application.h"
#include "ButtonComponent.h"
#include "CanvasComponent.h"
#include "DebugDrawModule.h"
#include "GameObject.h"
#include "ImageComponent.h"
#include "Scene.h"
#include "SceneModule.h"
#include "UILabelComponent.h"

#include "imgui.h"
#include <queue>

Transform2DComponent::Transform2DComponent(UID uid, GameObject* parent)
    : size(float2(400, 200)), pivot(float2(0.5f, 0.5f)), anchorsX(float2(0.5f, 0.5f)), anchorsY(float2(0.5f, 0.5f)),
      Component(uid, parent, "Transform 2D", COMPONENT_TRANSFORM_2D)
{
}

Transform2DComponent::Transform2DComponent(const rapidjson::Value& initialState, GameObject* parent)
    : Component(initialState, parent)
{
    if (initialState.HasMember("Position") && initialState["Position"].IsArray())
    {
        const rapidjson::Value& initPosition = initialState["Position"];
        position.x                           = initPosition[0].GetFloat();
        position.y                           = initPosition[1].GetFloat();
    }

    if (initialState.HasMember("Size") && initialState["Size"].IsArray())
    {
        const rapidjson::Value& initSize = initialState["Size"];
        size.x                           = initSize[0].GetFloat();
        size.y                           = initSize[1].GetFloat();
    }

    if (initialState.HasMember("Pivot") && initialState["Pivot"].IsArray())
    {
        const rapidjson::Value& initPivot = initialState["Pivot"];
        pivot.x                           = initPivot[0].GetFloat();
        pivot.y                           = initPivot[1].GetFloat();
    }

    if (initialState.HasMember("Anchors") && initialState["Anchors"].IsArray())
    {
        const rapidjson::Value& initAnchors = initialState["Anchors"];
        anchorsX.x                          = initAnchors[0].GetFloat();
        anchorsX.y                          = initAnchors[1].GetFloat();
        anchorsY.x                          = initAnchors[2].GetFloat();
        anchorsY.y                          = initAnchors[3].GetFloat();
    }
}

Transform2DComponent::~Transform2DComponent()
{
    if (parentTransform != nullptr) parentTransform->RemoveChild(this);

    UILabelComponent* uiLabel = parent->GetComponent<UILabelComponent*>();
    if (uiLabel) uiLabel->RemoveTransform();

    ImageComponent* image = parent->GetComponent<ImageComponent*>();
    if (image) image->RemoveTransform();

    ButtonComponent* button = parent->GetComponent<ButtonComponent*>();
    if (button) button->RemoveTransform();

    for (const auto& child : childTransforms)
    {
        child->RemoveParent();
    }
}

void Transform2DComponent::Init()
{
    GetCanvas();

    // GO is Canvas
    if (parent->GetComponent<CanvasComponent*>()) return;

    if (parentCanvas == nullptr)
    {
        GLOG("[Warning] You added a UI widget outside of a Canvas, which may lead to errors");
        return;
    }

    if (!IsRootTransform2D())
    {
        parentTransform = App->GetSceneModule()
                              ->GetScene()
                              ->GetGameObjectByUID(parent->GetParent())
                              ->GetComponent<Transform2DComponent*>();

        if (parentTransform) parentTransform->AddChildTransform(this);
    }
}

void Transform2DComponent::Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const
{
    Component::Save(targetState, allocator);

    rapidjson::Value valPosition(rapidjson::kArrayType);
    valPosition.PushBack(position.x, allocator);
    valPosition.PushBack(position.y, allocator);
    targetState.AddMember("Position", valPosition, allocator);

    rapidjson::Value valSize(rapidjson::kArrayType);
    valSize.PushBack(size.x, allocator);
    valSize.PushBack(size.y, allocator);
    targetState.AddMember("Size", valSize, allocator);

    rapidjson::Value valPivot(rapidjson::kArrayType);
    valPivot.PushBack(pivot.x, allocator);
    valPivot.PushBack(pivot.y, allocator);
    targetState.AddMember("Pivot", valPivot, allocator);

    rapidjson::Value valAnchors(rapidjson::kArrayType);
    valAnchors.PushBack(anchorsX.x, allocator);
    valAnchors.PushBack(anchorsX.y, allocator);
    valAnchors.PushBack(anchorsY.x, allocator);
    valAnchors.PushBack(anchorsY.y, allocator);
    targetState.AddMember("Anchors", valAnchors, allocator);
}

void Transform2DComponent::Clone(const Component* other)
{
    if (other->GetType() == ComponentType::COMPONENT_TRANSFORM_2D)
    {
        const Transform2DComponent* otherTransform = static_cast<const Transform2DComponent*>(other);
        position                                   = otherTransform->position;
        size                                       = otherTransform->size;
        pivot                                      = otherTransform->pivot;
        anchorsX                                   = otherTransform->anchorsX;
        anchorsY                                   = otherTransform->anchorsY;
        margins                                    = otherTransform->margins;
        previousMargins                            = otherTransform->previousMargins;
    }
    else
    {
        GLOG("It is not possible to clone a component of a different type!");
    }
}

void Transform2DComponent::RenderEditorInspector()
{
    Component::RenderEditorInspector();

    if (parentCanvas == nullptr)
    {
        ImGui::Text("No parent Canvas found. Ensure this UI element is under a Canvas.");
        return;
    }

    if (enabled)
    {
        ImGui::Text("Transform 2D");
        ImGui::PushItemWidth(75);

        // Position X / Left
        if (anchorsX.x == anchorsX.y)
        {
            if (ImGui::DragFloat("Pos X", &position.x, 0.1f)) UpdateParent3DTransform();
        }
        else
        {
            if (ImGui::DragFloat("Left", &margins.x, 0.1f)) OnLeftMarginChanged();
        }

        ImGui::SameLine();

        // Position Y / Top
        if (anchorsY.x == anchorsY.y)
        {
            if (ImGui::DragFloat("Pos Y", &position.y, 0.1f)) UpdateParent3DTransform();
        }
        else
        {
            if (ImGui::DragFloat("Top", &margins.z, 0.1f)) OnTopMarginChanged();
        }

        // Width / Right
        if (anchorsX.x == anchorsX.y)
        {
            if (ImGui::DragFloat("Width", &size.x, 0.1f)) OnSizeChanged();
        }
        else
        {
            if (ImGui::DragFloat("Right", &margins.y, 0.1f)) OnRightMarginChanged();
        }

        ImGui::SameLine();

        // Height / Bottom
        if (anchorsY.x == anchorsY.y)
        {
            if (ImGui::DragFloat("Height", &size.y, 0.1f)) OnSizeChanged();
        }
        else
        {
            if (ImGui::DragFloat("Bottom", &margins.w, 0.1f)) OnBottomMarginChanged();
        }

        ImGui::PopItemWidth();

        if (ImGui::DragFloat2("Pivot", &pivot[0], 0.01f, 0.0f, 1.0f))
        {
            OnAnchorsUpdated();
            OnSizeChanged();
        }
        ImGui::Spacing();
        ImGui::Text("Anchors");

        if (ImGui::DragFloat2("X-axis bounds", &anchorsX.x, 0.001f, 0.0f, 1.0f)) OnAnchorsUpdated();
        if (ImGui::DragFloat2("Y-axis bounds", &anchorsY.x, 0.001f, 0.0f, 1.0f)) OnAnchorsUpdated();

        ImGui::Separator();
    }
}

void Transform2DComponent::RenderWidgets() const
{
    DebugDrawModule* debugDraw = App->GetDebugDrawModule();

    float2 renderPos           = GetRenderingPosition();
    float2 renderSize          = size;

    if (parentCanvas && !parentCanvas->IsInWorldSpace())
    {
        float scale  = parentCanvas->GetScreenScale();
        renderPos   *= scale;
        renderSize  *= scale;
    }

    // Draw a square to show the width and height (around the widget)
    debugDraw->DrawLine(float3(renderPos.x - 1, renderPos.y + 1, 0), float3::unitX, renderSize.x + 2, float3(1, 1, 1));

    debugDraw->DrawLine(
        float3(renderPos.x + renderSize.x + 1, renderPos.y + 1, 0), -float3::unitY, renderSize.y + 2, float3(1, 1, 1)
    );

    debugDraw->DrawLine(
        float3(renderPos.x - 1, renderPos.y - renderSize.y - 1, 0), float3::unitX, renderSize.x + 2, float3(1, 1, 1)
    );

    debugDraw->DrawLine(float3(renderPos.x - 1, renderPos.y + 1, 0), -float3::unitY, renderSize.y + 2, float3(1, 1, 1));

    // Draw anchor points when selected
    if (App->GetSceneModule()->GetScene()->GetSelectedGameObject()->GetUID() == parent->GetUID())
    {
        const float scale = (parentCanvas && !parentCanvas->IsInWorldSpace()) ? parentCanvas->GetScreenScale() : 1.0f;

        // Top-left
        const float3 x1   = float3(GetAnchorXPos(anchorsX.x) * scale, GetAnchorYPos(anchorsY.y) * scale, 0);
        debugDraw->DrawCone(x1, float3(-10, 10, 0), 5, 1);

        // Top-right
        const float3 x2 = float3(GetAnchorXPos(anchorsX.y) * scale, GetAnchorYPos(anchorsY.y) * scale, 0);
        debugDraw->DrawCone(x2, float3(10, 10, 0), 5, 1);

        // Bottom-left
        const float3 y1 = float3(GetAnchorXPos(anchorsX.x) * scale, GetAnchorYPos(anchorsY.x) * scale, 0);
        debugDraw->DrawCone(y1, float3(-10, -10, 0), 5, 1);

        // Bottom-right
        const float3 y2 = float3(GetAnchorXPos(anchorsX.y) * scale, GetAnchorYPos(anchorsY.x) * scale, 0);
        debugDraw->DrawCone(y2, float3(10, -10, 0), 5, 1);
    }
}

void Transform2DComponent::UpdateParent3DTransform()
{
    // Get the parent local transform and update it according to the Transform2D. The parentTransform global
    // position is subtracted in order to get the anchor in a local position
    if (!parentCanvas || !parentCanvas->IsInWorldSpace()) return;

    float2 localPos;
    if (anchorsX.x == anchorsX.y)
        localPos.x = GetAnchorXPos(anchorsX.x) + position.x - parent->GetParentGlobalTransform().TranslatePart().x;
    else localPos.x = position.x;

    if (anchorsY.x == anchorsY.y)
        localPos.y = GetAnchorYPos(anchorsY.x) + position.y - parent->GetParentGlobalTransform().TranslatePart().y;
    else localPos.y = position.y;

    float4x4 transform = parent->GetLocalTransform();
    transform.SetTranslatePart(localPos.x, localPos.y, 0);

    transform2DUpdated = true;
    for (const auto& child : childTransforms)
    {
        child->transform2DUpdated = true;
    }
    parent->SetLocalTransform(transform);
}

void Transform2DComponent::OnTransform3DUpdated(const float4x4& globalTransform3D)
{
    // When the 3D transform of the gameObject is modified this is called
    if (transform2DUpdated || parentCanvas == nullptr)
    {
        // If the call was generated by updating the 2D transform, return
        transform2DUpdated = false;
        return;
    }

    // If the pivots are together just update the position. If they are not, get the position
    // at the center and update the margins
    if (anchorsX.x == anchorsX.y)
    {
        position.x = globalTransform3D.TranslatePart().x - GetAnchorXPos(anchorsX.x);
    }
    else
    {
        position.x = parent->GetLocalTransform().TranslatePart().x;
        UpdateHorizontalMargins();
    }

    if (anchorsY.x == anchorsY.y)
    {
        position.y = globalTransform3D.TranslatePart().y - GetAnchorYPos(anchorsY.x);
    }
    else
    {
        position.y = parent->GetLocalTransform().TranslatePart().y;
        UpdateVerticalMargins();
    }
}

float2 Transform2DComponent::GetRenderingPosition() const
{
    float2 pos = GetGlobalPosition();

    if (parentCanvas && !parentCanvas->IsInWorldSpace())
    {
        float scale  = parentCanvas->GetScreenScale();
        pos         *= scale;
    }

    float2 renderPos = float2(pos.x - (size.x * pivot.x), pos.y + (size.y * (1 - pivot.y)));
    return renderPos;
}

float2 Transform2DComponent::GetGlobalPosition() const
{
    if (parentCanvas && !parentCanvas->IsInWorldSpace())
    {
        float2 pos  = float2(0.0f, 0.0f);
        float scale = parentCanvas->GetScreenScale();
        return pos * scale;
    }

    return float2(parent->GetGlobalTransform().TranslatePart().x, parent->GetGlobalTransform().TranslatePart().y);
}

float2 Transform2DComponent::GetCenterPosition() const
{
    return float2(
        GetGlobalPosition().x + (size.x * (0.5f - pivot.x)), GetGlobalPosition().y + (size.y * (0.5f - pivot.y))
    );
};

void Transform2DComponent::GetCanvas()
{

    parentCanvas = parent->GetComponent<CanvasComponent*>();
    if (parentCanvas) return;

    // Search for a parent canvas iteratively
    std::queue<UID> parentQueue;
    parentQueue.push(parent->GetParent());

    Scene* scene =
        App->GetSceneModule()->GetScene(); // Save the scene here to not call for it in each iteration of the loops
    while (!parentQueue.empty())
    {
        const GameObject* currentParent = scene->GetGameObjectByUID(parentQueue.front());

        if (currentParent == nullptr) break; // If parent null it has reached the scene root

        CanvasComponent* canvas = currentParent->GetComponent<CanvasComponent*>();
        if (canvas != nullptr)
        {
            parentCanvas = canvas;
            break;
        }

        parentQueue.push(currentParent->GetParent());
        parentQueue.pop();
    }
}

bool Transform2DComponent::IsRootTransform2D() const
{
    if (parent->GetComponent<CanvasComponent*>()) return true;
    // Returns true if the parent is the canvas component
    return parentCanvas->GetParentUID() == parent->GetParent();
}

float Transform2DComponent::GetAnchorXPos(const float anchor) const
{
    // Gets the anchor global position
    float anchorPos = 0;

    if (parentCanvas == nullptr) return 0.0f;

    if (parentCanvas->IsInWorldSpace())
    {
        if (IsRootTransform2D())
        {
            // 0.5f because canvas pivot is always in the middle
            anchorPos =
                parent->GetParentGlobalTransform().TranslatePart().x + (parentCanvas->GetWidth() * (anchor - 0.5f));
        }
        else if (parentTransform)
        {
            anchorPos = parentTransform->GetGlobalPosition().x +
                        (parentTransform->size.x * (anchor - parentTransform->pivot.x));
        }
    }
    else
    {
        // Screen space: assume canvas is centered at (0,0)
        if (IsRootTransform2D())
        {
            anchorPos = parentCanvas->GetWidth() * (anchor - 0.5f);
        }
        else if (parentTransform)
        {
            anchorPos = parentTransform->GetGlobalPosition().x +
                        (parentTransform->size.x * (anchor - parentTransform->pivot.x));
        }
    }

    return anchorPos;
}

float Transform2DComponent::GetAnchorYPos(const float anchor) const
{
    // Gets the anchor global position
    float anchorPos = 0;

    if (parentCanvas == nullptr) return 0.0f;

    if (parentCanvas->IsInWorldSpace())
    {
        if (IsRootTransform2D())
        {
            // 0.5f because canvas pivot is always in the middle
            anchorPos =
                parent->GetParentGlobalTransform().TranslatePart().y + (parentCanvas->GetHeight() * (anchor - 0.5f));
        }
        else if (parentTransform)
        {
            anchorPos = parentTransform->GetGlobalPosition().y +
                        (parentTransform->size.y * (anchor - parentTransform->pivot.y));
        }
    }
    else
    {
        // Screen space: assume canvas is centered at (0,0)
        if (IsRootTransform2D())
        {
            anchorPos = parentCanvas->GetHeight() * (anchor - 0.5f);
        }
        else if (parentTransform)
        {
            anchorPos = parentTransform->GetGlobalPosition().y +
                        (parentTransform->size.y * (anchor - parentTransform->pivot.y));
        }
    }

    return anchorPos;
}

void Transform2DComponent::OnAnchorsUpdated()
{
    // when anchors are updated either update the positions respecting their distance or update the margins
    if (anchorsX.x == anchorsX.y)
    {
        previousPosition.x = position.x;
        position.x         = parent->GetGlobalTransform().TranslatePart().x - GetAnchorXPos(anchorsX.x);
    }
    else UpdateHorizontalMargins();

    if (anchorsY.x == anchorsY.y)
    {
        previousPosition.y = position.y;
        position.y         = parent->GetGlobalTransform().TranslatePart().y - GetAnchorYPos(anchorsY.x);
    }
    else UpdateVerticalMargins();
}

void Transform2DComponent::OnSizeChanged()
{
    // When the size is changed, update the children anchors and margins as well
    for (const auto& child : childTransforms)
    {
        child->AdaptToParentChanges();
    }
}

void Transform2DComponent::AdaptToParentChanges()
{
    OnAnchorsUpdated();

    if (anchorsX.x == anchorsX.y)
    {
        position.x = previousPosition.x;
    }
    else
    {
        margins.x = previousMargins.x;
        OnLeftMarginChanged();

        margins.y = previousMargins.y;
        OnRightMarginChanged();
    }

    if (anchorsY.x == anchorsY.y)
    {
        position.y = previousPosition.y;
    }
    else
    {
        margins.z = previousMargins.z;
        OnTopMarginChanged();

        margins.w = previousMargins.w;
        OnBottomMarginChanged();
    }

    UpdateParent3DTransform();
}

void Transform2DComponent::OnLeftMarginChanged()
{
    size.x =
        abs((GetAnchorXPos(anchorsX.x) + margins.x) -
            (parent->GetGlobalTransform().TranslatePart().x + (size.x * (1 - pivot.x))));

    position.x = ((GetAnchorXPos(anchorsX.x) + margins.x) + (GetAnchorXPos(anchorsX.y) + margins.y)) / 2;
    if (!IsRootTransform2D() && parentTransform) position.x -= parentTransform->GetGlobalPosition().x;
    position.x += (pivot.x - 0.5f) * (size.x);

    UpdateParent3DTransform();
    OnSizeChanged();
}

void Transform2DComponent::OnRightMarginChanged()
{
    size.x =
        abs((GetAnchorXPos(anchorsX.y) + margins.y) -
            (parent->GetGlobalTransform().TranslatePart().x - (size.x * pivot.x)));

    position.x = ((GetAnchorXPos(anchorsX.x) + margins.x) + (GetAnchorXPos(anchorsX.y) + margins.y)) / 2;
    if (!IsRootTransform2D() && parentTransform) position.x -= parentTransform->GetGlobalPosition().x;
    position.x += (pivot.x - 0.5f) * (size.x);

    UpdateParent3DTransform();
    OnSizeChanged();
}

void Transform2DComponent::OnTopMarginChanged()
{
    size.y =
        abs((GetAnchorYPos(anchorsY.y) + margins.z) -
            (parent->GetGlobalTransform().TranslatePart().y - (size.y * pivot.y)));

    position.y = ((GetAnchorYPos(anchorsY.y) + margins.z) + (GetAnchorYPos(anchorsY.x) + margins.w)) / 2;
    if (!IsRootTransform2D() && parentTransform) position.y -= parentTransform->GetGlobalPosition().y;
    position.y += (pivot.y - 0.5f) * (size.y);

    UpdateParent3DTransform();
    OnSizeChanged();
}

void Transform2DComponent::OnBottomMarginChanged()
{
    size.y =
        abs((GetAnchorYPos(anchorsY.x) + margins.w) -
            (parent->GetGlobalTransform().TranslatePart().y + (size.y * (1 - pivot.y))));

    position.y = ((GetAnchorYPos(anchorsY.y) + margins.z) + (GetAnchorYPos(anchorsY.x) + margins.w)) / 2;
    if (!IsRootTransform2D() && parentTransform) position.y -= parentTransform->GetGlobalPosition().y;
    position.y += (pivot.y - 0.5f) * (size.y);

    UpdateParent3DTransform();
    OnSizeChanged();
}

void Transform2DComponent::UpdateHorizontalMargins()
{
    previousMargins.x = margins.x;
    previousMargins.y = margins.y;

    margins.x = (parent->GetGlobalTransform().TranslatePart().x - (size.x * pivot.x)) - GetAnchorXPos(anchorsX.x);
    margins.y = (parent->GetGlobalTransform().TranslatePart().x + (size.x * (1 - pivot.x))) - GetAnchorXPos(anchorsX.y);
}

void Transform2DComponent::UpdateVerticalMargins()
{
    previousMargins.z = margins.z;
    previousMargins.w = margins.w;

    margins.z = (parent->GetGlobalTransform().TranslatePart().y + (size.y * (1 - pivot.y))) - GetAnchorYPos(anchorsY.y);
    margins.w = (parent->GetGlobalTransform().TranslatePart().y - (size.y * pivot.y)) - GetAnchorYPos(anchorsY.x);
}

void Transform2DComponent::RemoveChild(Transform2DComponent* child)
{
    const auto& it = std::find(childTransforms.begin(), childTransforms.end(), child);
    if (it != childTransforms.end()) childTransforms.erase(it);
}

void Transform2DComponent::OnParentChange()
{
    if (parentTransform) parentTransform->RemoveChild(this);

    // Get the canvas again, just in case the object is now outside of a canvas
    parentCanvas    = nullptr;
    parentTransform = nullptr;
    GetCanvas();

    if (parentCanvas && !IsRootTransform2D())
    {
        parentTransform = App->GetSceneModule()
                              ->GetScene()
                              ->GetGameObjectByUID(parent->GetParent())
                              ->GetComponent<Transform2DComponent*>();

        if (parentTransform) parentTransform->AddChildTransform(this);
        else GLOG("[WARNING] You are assigning a Transform2D as a child of a gameObject with no Transform2D");
    }
}

void Transform2DComponent::OnCanvasRenderModeChanged(CanvasComponent::CanvasRenderMode newMode)
{
    if (!parentCanvas) return;

    float2 worldPos = GetAbsoluteWorldPosition();
    float scale     = parentCanvas->GetScreenScale();

    float2 newLocalPos = worldPos;

    if (parent != parentCanvas->GetParent())
    {
        float2 parentWorldPos  = parent->GetComponent<Transform2DComponent*>()->GetAbsoluteWorldPosition();
        newLocalPos           -= parentWorldPos;
    }

    if (newMode == CanvasComponent::CanvasRenderMode::ScreenSpaceOverlay)
    {
        position = newLocalPos / scale;
    }
    else if (newMode == CanvasComponent::CanvasRenderMode::WorldSpace)
    {
        position = newLocalPos * scale;
    }

    UpdateParent3DTransform();
}



float2 Transform2DComponent::GetAbsoluteWorldPosition() const
{
    return float2(parent->GetGlobalTransform().TranslatePart().x, parent->GetGlobalTransform().TranslatePart().y);
}