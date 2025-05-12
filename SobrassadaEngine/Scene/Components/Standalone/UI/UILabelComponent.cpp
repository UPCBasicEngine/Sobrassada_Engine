#include "UILabelComponent.h"

#include "Application.h"
#include "CameraModule.h"
#include "CanvasComponent.h"
#include "LibraryModule.h"
#include "ResourceFont.h"
#include "ResourcesModule.h"
#include "GameObject.h"
#include "Scene.h"
#include "SceneModule.h"
#include "ShaderModule.h"
#include "TextManager.h"
#include "Transform2DComponent.h"
#include "WindowModule.h"
#include "HashString.h"

#include "glew.h"
#include "imgui.h"

UILabelComponent::UILabelComponent(UID uid, GameObject* parent)
    : text("Le Sobrassada"), Component(uid, parent, "Label", COMPONENT_LABEL)
{
    fontData = new TextManager::FontData();

    // Set the default font resource
    fontType = static_cast<ResourceFont*>(
        App->GetResourcesModule()->RequestResource(App->GetLibraryModule()->GetFontMap().at(HashString("Roboto-Regular")))
    );
}

UILabelComponent::UILabelComponent(const rapidjson::Value& initialState, GameObject* parent)
    : Component(initialState, parent)
{
    fontData            = new TextManager::FontData();

    const char* textPtr = initialState["Text"].GetString();
    strcpy_s(text, sizeof(text), textPtr);
    fontSize = initialState["FontSize"].GetInt();

    if (initialState.HasMember("FontUID"))
    {
        fontType =
            static_cast<ResourceFont*>(App->GetResourcesModule()->RequestResource(initialState["FontUID"].GetUint64()));
    }
    else
    {
        // To prevent crashes in older saved scenes, load the default one if there is no font saved
        GLOG("[Warning] No font type found for the component %s in the saved scene", name);
        fontType = static_cast<ResourceFont*>(
            App->GetResourcesModule()->RequestResource(App->GetLibraryModule()->GetFontMap().at(HashString("Roboto-Regular")))
        );
    }

    if (initialState.HasMember("FontColor") && initialState["FontColor"].IsArray())
    {
        const rapidjson::Value& initFontColor = initialState["FontColor"];
        fontColor.x                           = initFontColor[0].GetFloat();
        fontColor.y                           = initFontColor[1].GetFloat();
        fontColor.z                           = initFontColor[2].GetFloat();
    }
}

UILabelComponent::~UILabelComponent()
{
    fontData->Clean();
    delete fontData;

    App->GetResourcesModule()->ReleaseResource(fontType);

    if (vbo != 0) glDeleteBuffers(1, &vbo);
    if (vao != 0) glDeleteVertexArrays(1, &vao);
}

void UILabelComponent::Init()
{
    transform2D  = parent->GetComponent<Transform2DComponent*>();
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

        if (parentCanvas == nullptr) GLOG("[WARNING] Label has no parent canvas, it won't be rendered");
    }

    fontData->Clean();
    fontData->Init(fontType->GetFilepath().c_str(), fontSize);
    InitBuffers();
}

void UILabelComponent::Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const
{
    Component::Save(targetState, allocator);

    std::string textString(text);
    targetState.AddMember("Text", rapidjson::Value(textString.c_str(), allocator), allocator);
    targetState.AddMember("FontSize", fontSize, allocator);

    targetState.AddMember("FontUID", fontType->GetUID(), allocator);

    rapidjson::Value valFontColor(rapidjson::kArrayType);
    valFontColor.PushBack(fontColor.x, allocator);
    valFontColor.PushBack(fontColor.y, allocator);
    valFontColor.PushBack(fontColor.z, allocator);
    targetState.AddMember("FontColor", valFontColor, allocator);
}

void UILabelComponent::Clone(const Component* other)
{
    // It will have to look for the canvas here probably, seems better to do that in a separate function
    if (other->GetType() == ComponentType::COMPONENT_LABEL)
    {
        const UILabelComponent* otherLabel = static_cast<const UILabelComponent*>(other);
        enabled                            = otherLabel->enabled;

        strcpy_s(text, sizeof(text), otherLabel->text);
        fontSize  = otherLabel->fontSize;
        fontColor = otherLabel->fontColor;

        fontType  = otherLabel->fontType;
        fontType->AddReference();

        fontData->Clean();
        fontData->Init(fontType->GetFilepath().c_str(), fontSize);
    }
    else
    {
        GLOG("It is not possible to clone a component of a different type!");
    }
}

void UILabelComponent::RenderDebug(float deltaTime)
{

}

void UILabelComponent::RenderUI(const float4x4& view, const float4x4 proj) const
{
    if (parentCanvas == nullptr || !IsEffectivelyEnabled()) return;

    float width = 0;
    float3 startPos;
    if (transform2D)
    {
        startPos = float3(transform2D->GetRenderingPosition(), 0) - parent->GetGlobalTransform().TranslatePart();
        width = transform2D->size.x;
    }
    glUniformMatrix4fv(0, 1, GL_TRUE, parent->GetGlobalTransform().ptr());
    glUniformMatrix4fv(1, 1, GL_TRUE, view.ptr());
    glUniformMatrix4fv(2, 1, GL_TRUE, proj.ptr());

    glUniform3fv(3, 1, fontColor.ptr()); // Font color
    glUniform1ui(5, 0);                  // Tell the shader this widget is a label = 0

    glBindVertexArray(vao);
    TextManager::RenderText(*fontData, text, startPos, vbo, width);
}

void UILabelComponent::RenderEditorInspector()
{
    Component::RenderEditorInspector();

    if (enabled)
    {
        ImGui::Text("Label");
        ImGui::InputTextMultiline("Label Text", &text[0], sizeof(text));

        if (ImGui::InputInt("Font Size", &fontSize))
        {
            if (fontSize < 1) fontSize = 1;
            OnFontChange();
        }

        const char* preview = "Arial";
        if (ImGui::BeginCombo("Font Type", fontData->fontName.c_str()))
        {
            int selectedItemIndex = -1;
            unsigned int i        = 0;
            for (const auto& font : App->GetLibraryModule()->GetFontMap())
            {
                const bool is_selected = (selectedItemIndex == i);
                if (ImGui::Selectable(font.first.c_str(), is_selected))
                {
                    selectedItemIndex = i;

                    fontType = static_cast<ResourceFont*>(App->GetResourcesModule()->RequestResource(font.second));
                    fontData->Clean();
                    fontData->Init(fontType->GetFilepath().c_str(), fontSize);
                }

                if (is_selected) ImGui::SetItemDefaultFocus();
                ++i;
            }
            ImGui::EndCombo();
        }

        ImGui::ColorEdit3("Font color", fontColor.ptr());
    }
}

void UILabelComponent::InitBuffers()
{
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 24, NULL, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void UILabelComponent::OnFontChange()
{
    fontData->Clean();
    fontData->Init(fontType->GetFilepath().c_str(), fontSize);
}