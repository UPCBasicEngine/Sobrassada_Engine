#include "CanvasComponent.h"

#include "Application.h"
#include "ButtonComponent.h"
#include "CameraModule.h"
#include "CanvasScalerComponent.h"
#include "DebugDrawModule.h"
#include "GameObject.h"
#include "GameUIModule.h"
#include "ImageComponent.h"
#include "SceneModule.h"
#include "ShaderModule.h"
#include "Transform2DComponent.h"
#include "UILabelComponent.h"
#include "WindowModule.h"

#include "glew.h"
#include "imgui.h"
#include <queue>

CanvasComponent::CanvasComponent(UID uid, GameObject* parent) : Component(uid, parent, "Canvas", COMPONENT_CANVAS)
{
}

CanvasComponent::CanvasComponent(const rapidjson::Value& initialState, GameObject* parent)
    : Component(initialState, parent)
{
    renderMode = static_cast<CanvasRenderMode>(initialState["RenderMode"].GetInt());
}

CanvasComponent::~CanvasComponent()
{
    App->GetGameUIModule()->RemoveCanvas(this);
}

void CanvasComponent::Init()
{
    transform2D = parent->GetComponent<Transform2DComponent*>();
    if (transform2D == nullptr)
    {
        parent->CreateComponent(COMPONENT_TRANSFORM_2D);
        transform2D = parent->GetComponent<Transform2DComponent*>();
    }

    if (parent->GetComponent<CanvasScalerComponent*>() == nullptr)
    {
        parent->CreateComponent(COMPONENT_CANVAS_SCALER);
    }

    App->GetGameUIModule()->AddCanvas(this);
    UpdateBoundingBox();
}

void CanvasComponent::Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const
{
    Component::Save(targetState, allocator);

    // Render mode: 0 = Overlay, 1 = WorldSpace
    targetState.AddMember("RenderMode", static_cast<int>(renderMode), allocator);
}

void CanvasComponent::Clone(const Component* other)
{
    if (other->GetType() == ComponentType::COMPONENT_CANVAS)
    {
        const CanvasComponent* otherCanvas = static_cast<const CanvasComponent*>(other);
        renderMode                         = otherCanvas->renderMode;
    }
    else
    {
        GLOG("It is not possible to clone a component of a different type!");
    }
}

void CanvasComponent::Update(float deltaTime)
{
    if (!IsEffectivelyEnabled()) return;
    if (renderMode != CanvasRenderMode::ScreenSpaceOverlay) return;

    // Update canvas if window size changed (only in overlay space)
    const auto& size = App->GetSceneModule()->GetScene()->GetWindowSize();
    float newWidth   = std::get<0>(size);
    float newHeight  = std::get<1>(size);
    if (GetWidth() != newWidth || GetHeight() != newHeight)
    {
        OnWindowResize(newWidth, newHeight);
    }
}

// Draw the canvas area in WorldSpace
void CanvasComponent::RenderDebug(float deltaTime)
{
    float3 defaultColor = float3(1, 1, 1);
    RenderDebug(deltaTime, defaultColor);
}

void CanvasComponent::RenderDebug(float deltaTime, const float3& color)
{
    if (!IsEffectivelyEnabled()) return;

    float x     = parent->GetGlobalTransform().TranslatePart().x;
    float y     = parent->GetGlobalTransform().TranslatePart().y;

    float2 size = transform2D ? transform2D->GetScaledSize() : float2::zero;

    App->GetDebugDrawModule()->DrawLine(float3(x - size.x / 2, y + size.y / 2, 0), float3::unitX, size.x, color);
    App->GetDebugDrawModule()->DrawLine(float3(x - size.x / 2, y - size.y / 2, 0), float3::unitX, size.x, color);
    App->GetDebugDrawModule()->DrawLine(float3(x - size.x / 2, y + size.y / 2, 0), -float3::unitY, size.y, color);
    App->GetDebugDrawModule()->DrawLine(float3(x + size.x / 2, y + size.y / 2, 0), -float3::unitY, size.y, color);
}

// Renders all UI elements under this canvas using appropriate view/projection based on render mode
void CanvasComponent::RenderUI()
{
    if (!IsEffectivelyEnabled()) return;

    const int uiProgram = App->GetShaderModule()->GetUIWidgetProgram();
    if (uiProgram == -1)
    {
        GLOG("Error with UI Program");
        return;
    }
    glUseProgram(uiProgram);

    float4x4 view = float4x4::identity;
    float4x4 proj = float4x4::identity;

    if (renderMode == CanvasRenderMode::WorldSpace)
    {
        view = App->GetCameraModule()->GetViewMatrix();
        proj = App->GetCameraModule()->GetProjectionMatrix();
    }
    else // ScreenSpaceOverlay
    {
        proj = float4x4::D3DOrthoProjLH(-1, 1, GetWidth(), GetHeight());
    }

    for (const GameObject* child : sortedChildren)
    {
        if (!child->IsGloballyEnabled()) continue;

        Transform2DComponent* transform = child->GetComponent<Transform2DComponent*>();
        if (transform) transform->RenderWidgets();

        UILabelComponent* uiLabel = child->GetComponent<UILabelComponent*>();
        if (uiLabel) uiLabel->RenderUI(view, proj);

        ImageComponent* image = child->GetComponent<ImageComponent*>();
        if (image) image->RenderUI(view, proj);
    }
}

// Update canvas size and bounding box when in Screen Space Overlay mode
void CanvasComponent::OnWindowResize(const float width, const float height)
{
    if (renderMode != CanvasRenderMode::ScreenSpaceOverlay || !transform2D) return;

    if (parent->GetComponent<CanvasScalerComponent*>()) return;

    transform2D->size = float2(width, height);
    UpdateBoundingBox();
}

void CanvasComponent::UpdateChildren()
{
    if (!IsEffectivelyEnabled()) return;

    // TODO: Right now this updates the children list every frame in case they are reordered in hierarchy.
    // To be more optimal, this could be called only when a gameObject is dragged around the hierarchy
    sortedChildren.clear();

    std::queue<UID> children;

    for (const UID child : parent->GetChildren())
    {
        children.push(child);
    }

    while (!children.empty())
    {
        const GameObject* currentObject = App->GetSceneModule()->GetScene()->GetGameObjectByUID(children.front());
        sortedChildren.push_back(currentObject);
        children.pop();

        for (const UID child : currentObject->GetChildren())
        {
            children.push(child);
        }
    }
}

void CanvasComponent::UpdateMousePosition(const float2& mousePos)
{
    if (!IsEffectivelyEnabled()) return;

    // Only interact with elements if canvas is in screen mode
    if (renderMode == CanvasRenderMode::WorldSpace) return;

    hoveredButton    = nullptr;
    bool buttonFound = false;

    for (int i = (int)sortedChildren.size() - 1; i >= 0; --i)
    {
        // Update all buttons
        ButtonComponent* currentButton = sortedChildren[i]->GetComponent<ButtonComponent*>();
        if (currentButton && currentButton->UpdateMousePosition(mousePos, buttonFound))
        {
            hoveredButton = currentButton;
            buttonFound   = true;
        }
    }
}

void CanvasComponent::RenderEditorInspector()
{
    Component::RenderEditorInspector();

    if (enabled)
    {
        ImGui::Text("Canvas");

        const char* modes[] = {"Screen Space - Overlay", "World Space"};
        int currentMode     = static_cast<int>(renderMode);
        if (ImGui::Combo("Render Mode", &currentMode, modes, IM_ARRAYSIZE(modes)))
        {
            ChangeRenderMode(static_cast<CanvasRenderMode>(currentMode));
        }
    }
}

void CanvasComponent::OnMouseButtonPressed() const
{
    if (hoveredButton) hoveredButton->OnClick();
}

void CanvasComponent::OnMouseButtonReleased() const
{
    if (hoveredButton) hoveredButton->OnRelease();
}

bool CanvasComponent::IsInWorldSpace() const
{
    return renderMode == CanvasRenderMode::WorldSpace;
}

void CanvasComponent::UpdateBoundingBox()
{
    float2 center      = transform2D->GetGlobalPosition();

    float maxHalfSize  = 250.0f;
    localComponentAABB = AABB(
        float3(center.x - maxHalfSize, center.y - maxHalfSize, -0.1f),
        float3(center.x + maxHalfSize, center.y + maxHalfSize, 0.1f)
    );
}

float CanvasComponent::GetWidth() const
{
    return transform2D ? transform2D->size.x : 0.0f;
}
float CanvasComponent::GetHeight() const
{
    return transform2D ? transform2D->size.y : 0.0f;
}

void CanvasComponent::ChangeRenderMode(CanvasRenderMode newMode)
{
    GLOG("Canvas switching to: %d", newMode);

    // Store the world position of each child before switching
    std::unordered_map<UID, float2> savedWorldPositions;

    for (UID childUID : parent->GetChildren())
    {
        GameObject* child = App->GetSceneModule()->GetScene()->GetGameObjectByUID(childUID);
        if (!child) continue;

        if (Transform2DComponent* transform = child->GetComponent<Transform2DComponent*>())
        {
            savedWorldPositions[child->GetUID()] = transform->GetAbsoluteWorldPosition();
        }
    }

    renderMode = newMode;
    UpdateBoundingBox();

    // Update each child's local position based on their previous world position
    for (UID childUID : parent->GetChildren())
    {
        GameObject* child = App->GetSceneModule()->GetScene()->GetGameObjectByUID(childUID);
        if (!child) continue;

        if (Transform2DComponent* transform = child->GetComponent<Transform2DComponent*>())
        {
            transform->OnCanvasRenderModeChanged(newMode, savedWorldPositions[child->GetUID()]);
        }
    }
}

float CanvasComponent::GetScreenScale() const
{
    CanvasScalerComponent* scaler = parent->GetComponent<CanvasScalerComponent*>();
    return scaler ? scaler->GetScale() : 1.0f;
}
