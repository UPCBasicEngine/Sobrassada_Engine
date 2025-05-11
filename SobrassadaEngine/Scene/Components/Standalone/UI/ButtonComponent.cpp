#include "ButtonComponent.h"

#include "Application.h"
#include "CanvasComponent.h"
#include "GameObject.h"
#include "ImageComponent.h"
#include "Transform2DComponent.h"

#include "imgui.h"

ButtonComponent::ButtonComponent(UID uid, GameObject* parent)
    : defaultColor(float3(1.0f, 1.0f, 1.0f)), hoverColor(float3(0.0f, 1.0f, 1.0f)),
      clickedColor(float3(1.0f, 1.0f, 0.0f)), disabledColor(float3(0.5f, 0.5f, 0.5f)), isInteractable(true),
      Component(uid, parent, "Button", COMPONENT_BUTTON)
{
}

ButtonComponent::ButtonComponent(const rapidjson::Value& initialState, GameObject* parent)
    : Component(initialState, parent)
{
    if (initialState.HasMember("IsInteractable"))
    {
        isInteractable = initialState["IsInteractable"].GetBool();
    }

    if (initialState.HasMember("DefaultColor") && initialState["DefaultColor"].IsArray())
    {
        const rapidjson::Value& initColor = initialState["DefaultColor"];
        defaultColor.x                    = initColor[0].GetFloat();
        defaultColor.y                    = initColor[1].GetFloat();
        defaultColor.z                    = initColor[2].GetFloat();
    }

    if (initialState.HasMember("HoverColor") && initialState["HoverColor"].IsArray())
    {
        const rapidjson::Value& initColor = initialState["HoverColor"];
        hoverColor.x                      = initColor[0].GetFloat();
        hoverColor.y                      = initColor[1].GetFloat();
        hoverColor.z                      = initColor[2].GetFloat();
    }

    if (initialState.HasMember("ClickedColor") && initialState["ClickedColor"].IsArray())
    {
        const rapidjson::Value& initColor = initialState["ClickedColor"];
        clickedColor.x                    = initColor[0].GetFloat();
        clickedColor.y                    = initColor[1].GetFloat();
        clickedColor.z                    = initColor[2].GetFloat();
    }

    if (initialState.HasMember("DisabledColor") && initialState["DisabledColor"].IsArray())
    {
        const rapidjson::Value& initColor = initialState["DisabledColor"];
        disabledColor.x                   = initColor[0].GetFloat();
        disabledColor.y                   = initColor[1].GetFloat();
        disabledColor.z                   = initColor[2].GetFloat();
    }
}

ButtonComponent::~ButtonComponent()
{
    ClearAllCallbacks();
}

void ButtonComponent::Init()
{
    transform2D = parent->GetComponent<Transform2DComponent*>();
    if (transform2D == nullptr)
    {
        parent->CreateComponent(COMPONENT_TRANSFORM_2D);
        transform2D = parent->GetComponent<Transform2DComponent*>();
    }

    parentCanvas = transform2D->GetParentCanvas();

    if (parentCanvas == nullptr)
    {
        // Try to get it again, just in case the transform was created later
        transform2D->GetCanvas();
        parentCanvas = transform2D->GetParentCanvas();

        if (parentCanvas == nullptr) GLOG("[WARNING] Button has no parent canvas, it won't be rendered");
    }

    // Get the image
    image = parent->GetComponent<ImageComponent*>();
    if (image == nullptr)
    {
        parent->CreateComponent(COMPONENT_IMAGE);
        image = parent->GetComponent<ImageComponent*>();
    }
}

void ButtonComponent::Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const
{
    Component::Save(targetState, allocator);

    targetState.AddMember("IsInteractable", isInteractable, allocator);

    rapidjson::Value valColorDefault(rapidjson::kArrayType);
    valColorDefault.PushBack(defaultColor.x, allocator);
    valColorDefault.PushBack(defaultColor.y, allocator);
    valColorDefault.PushBack(defaultColor.z, allocator);
    targetState.AddMember("DefaultColor", valColorDefault, allocator);

    rapidjson::Value valColorHover(rapidjson::kArrayType);
    valColorHover.PushBack(hoverColor.x, allocator);
    valColorHover.PushBack(hoverColor.y, allocator);
    valColorHover.PushBack(hoverColor.z, allocator);
    targetState.AddMember("HoverColor", valColorHover, allocator);

    rapidjson::Value valColorClicked(rapidjson::kArrayType);
    valColorClicked.PushBack(clickedColor.x, allocator);
    valColorClicked.PushBack(clickedColor.y, allocator);
    valColorClicked.PushBack(clickedColor.z, allocator);
    targetState.AddMember("ClickedColor", valColorClicked, allocator);

    rapidjson::Value valColorDisabled(rapidjson::kArrayType);
    valColorDisabled.PushBack(disabledColor.x, allocator);
    valColorDisabled.PushBack(disabledColor.y, allocator);
    valColorDisabled.PushBack(disabledColor.z, allocator);
    targetState.AddMember("DisabledColor", valColorDisabled, allocator);
}

void ButtonComponent::Clone(const Component* other)
{
    // It will have to look for the canvas here probably, seems better to do that in a separate function
    if (other->GetType() == COMPONENT_BUTTON)
    {
        const ButtonComponent* otherButton = static_cast<const ButtonComponent*>(other);
        enabled                            = otherButton->enabled;
        wasEnabled                         = otherButton->wasEnabled;
        isInteractable                     = otherButton->isInteractable;

        defaultColor                       = otherButton->defaultColor;
        hoverColor                         = otherButton->hoverColor;
        clickedColor                       = otherButton->clickedColor;
        disabledColor                      = otherButton->disabledColor;
    }
    else
    {
        GLOG("It is not possible to clone a component of a different type!");
    }
}

void ButtonComponent::RenderDebug(float deltaTime)
{
}

void ButtonComponent::RenderEditorInspector()
{
    Component::RenderEditorInspector();

    ImGui::SeparatorText("Button");

    if (ImGui::Checkbox("Interactable", &isInteractable)) OnInteractionChange();
    if (ImGui::ColorEdit3("Default color", defaultColor.ptr())) image->SetColor(defaultColor);
    ImGui::ColorEdit3("Hover color", hoverColor.ptr());
    ImGui::ColorEdit3("Clicked color", clickedColor.ptr());
    ImGui::ColorEdit3("Disabled color", disabledColor.ptr());
}

bool ButtonComponent::UpdateMousePosition(const float2& mousePos, bool dismiss)
{
    if (!isInteractable || !IsEffectivelyEnabled()) return false;

    if (!dismiss && IsWithinBounds(mousePos))
    {
        if (!isHovered)
        {
            // On mouse enter
            if (image) image->SetColor(hoverColor);
            isHovered = true;
        }
        return true;
    }
    else
    {
        if (isHovered)
        {
            // On mouse exit
            if (image) image->SetColor(defaultColor);
            isHovered = false;
        }
        return false;
    }
}

void ButtonComponent::OnClick()
{
    //GLOG("Clicked button!");
    onClickDispatcher.Call();
    if (image) image->SetColor(clickedColor);
}

void ButtonComponent::OnRelease() const
{
    if (image) image->SetColor(hoverColor);
}

bool ButtonComponent::IsWithinBounds(const float2& pos) const
{
    if (transform2D == nullptr || parentCanvas == nullptr) return false;

    // Get the position in button local space
    const float2 canvasCenter = float2(parentCanvas->GetWidth(), parentCanvas->GetHeight()) * 0.5f;
    const float2 localPos     = pos - (transform2D->GetCenterPosition() + canvasCenter);
    const float3 localRotated =
        parent->GetGlobalTransform().RotatePart().Inverted() * float3(localPos.x, localPos.y, 0.0f);

    // Check if it is inside the button's AABB in local space
    return abs(localRotated.x) <= transform2D->size.x * 0.5f && abs(localRotated.y) <= transform2D->size.y * 0.5f;
}

std::list<Delegate<void>>::iterator ButtonComponent::AddOnClickCallback(Delegate<void> newDelegate)
{
    return onClickDispatcher.SubscribeCallback(std::move(newDelegate));
}

void ButtonComponent::RemoveOnClickCallback(std::list<Delegate<void>>::iterator delegate)
{
    onClickDispatcher.SafeRemoveCallback(delegate);
}

void ButtonComponent::OnInteractionChange() const
{
    if (image == nullptr) return;

    if (isInteractable) image->SetColor(defaultColor);
    else image->SetColor(disabledColor);
}

void ButtonComponent::ClearAllCallbacks()
{
    onClickDispatcher.Clear();
}