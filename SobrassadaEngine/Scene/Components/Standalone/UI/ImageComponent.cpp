#include "ImageComponent.h"

#include "Application.h"
#include "CanvasComponent.h"
#include "EditorUIModule.h"
#include "GameObject.h"
#include "LibraryModule.h"
#include "OpenGLModule.h"
#include "ResourcesModule.h"
#include "SceneModule.h"
#include "Transform2DComponent.h"

#include "ImGui.h"
#include "glew.h"

ImageComponent::ImageComponent(UID uid, GameObject* parent)
    : color(float3(1.0f, 1.0f, 1.0f)), Component(uid, parent, "Image", COMPONENT_IMAGE)
{
    // Set default texture
    texture = static_cast<ResourceTexture*>(
        App->GetResourcesModule()->RequestResource(App->GetLibraryModule()->GetTextureMap().at(HashString("DefaultTexture")))
    );
}

ImageComponent::ImageComponent(const rapidjson::Value& initialState, GameObject* parent)
    : Component(initialState, parent)
{
    if (initialState.HasMember("TextureUID"))
    {
        texture = static_cast<ResourceTexture*>(
            App->GetResourcesModule()->RequestResource(initialState["TextureUID"].GetUint64())
        );
    }
    else
    {
        // To prevent crashes, load the default one if there is no texture saved
        GLOG("[WARNING] No texture UID found for the UI image %s in the saved scene", name);
        texture = static_cast<ResourceTexture*>(
            App->GetResourcesModule()->RequestResource(App->GetLibraryModule()->GetTextureMap().at(HashString("DefaultTexture")))
        );
    }

    if (initialState.HasMember("Color") && initialState["Color"].IsArray())
    {
        const rapidjson::Value& initColor = initialState["Color"];
        color.x                           = initColor[0].GetFloat();
        color.y                           = initColor[1].GetFloat();
        color.z                           = initColor[2].GetFloat();
    }

    if (initialState.HasMember("MatchParentSize")) matchParentSize = initialState["MatchParentSize"].GetBool();
}

ImageComponent::~ImageComponent()
{
    ClearBuffers();
}

void ImageComponent::Init()
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

        if (parentCanvas == nullptr) GLOG("[WARNING] Image has no parent canvas, it won't be rendered");
    }

    InitBuffers();
}
void ImageComponent::Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const
{
    Component::Save(targetState, allocator);

    targetState.AddMember("TextureUID", texture->GetUID(), allocator);
    targetState.AddMember("MatchParentSize", matchParentSize, allocator);

    rapidjson::Value valColor(rapidjson::kArrayType);
    valColor.PushBack(color.x, allocator);
    valColor.PushBack(color.y, allocator);
    valColor.PushBack(color.z, allocator);
    targetState.AddMember("Color", valColor, allocator);
}
void ImageComponent::Clone(const Component* other)
{
    // It will have to look for the canvas here probably, seems better to do that in a separate function
    if (other->GetType() == COMPONENT_IMAGE)
    {
        const ImageComponent* otherImage = static_cast<const ImageComponent*>(other);
        enabled                          = otherImage->enabled;

        color                            = otherImage->color;
        texture                          = otherImage->texture;
        texture->AddReference();
        bindlessUID = glGetTextureHandleARB(texture->GetTextureID());
        glMakeTextureHandleResidentARB(bindlessUID);
    }
    else
    {
        GLOG("It is not possible to clone a component of a different type!");
    }
}

void ImageComponent::RenderDebug(float deltaTime)
{
}

void ImageComponent::Update(float deltaTime)
{
    if (matchParentSize) MatchParentSize();
}

void ImageComponent::RenderEditorInspector()
{
    Component::RenderEditorInspector();

    ImGui::SeparatorText("Image");

    ImGui::Text("Source texture: ");
    ImGui::SameLine();
    ImGui::Text(texture->GetName().c_str());

    ImGui::Checkbox("Match Parent Size", &matchParentSize);

    if (ImGui::Button("Select texture"))
    {
        ImGui::OpenPopup(CONSTANT_TEXTURE_SELECT_DIALOG_ID);
    }
    if (ImGui::IsPopupOpen(CONSTANT_TEXTURE_SELECT_DIALOG_ID))
    {
        ChangeTexture(App->GetEditorUIModule()->RenderResourceSelectDialog<UID>(
            CONSTANT_TEXTURE_SELECT_DIALOG_ID, App->GetLibraryModule()->GetTextureMap(), INVALID_UID
        ));
    }

    ImGui::ColorEdit3("Image color", color.ptr());
}

void ImageComponent::RenderUI(const float4x4& view, const float4x4& proj) const
{
    if (parentCanvas == nullptr || transform2D == nullptr || !IsEffectivelyEnabled()) return;

    const float3 startPos =
        float3(transform2D->GetRenderingPosition(), 0) - parent->GetGlobalTransform().TranslatePart();
    glUniformMatrix4fv(0, 1, GL_TRUE, parent->GetGlobalTransform().ptr());
    glUniformMatrix4fv(1, 1, GL_TRUE, view.ptr());
    glUniformMatrix4fv(2, 1, GL_TRUE, proj.ptr());

    glUniform3fv(3, 1, color.ptr());
    glUniform1ui(5, 1); // Tell the shader this widget is an image = 1

    glBindVertexArray(vao);

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    const float vertices[] = {
        startPos.x,
        startPos.y,
        0.0f,
        0.0f,
        startPos.x,
        startPos.y - transform2D->size.y,
        0.0f,
        1.0f,
        startPos.x + transform2D->size.x,
        startPos.y - transform2D->size.y,
        1.0f,
        1.0f,

        startPos.x + transform2D->size.x,
        startPos.y,
        1.0f,
        0.0f,
        startPos.x,
        startPos.y,
        0.0f,
        0.0f,
        startPos.x + transform2D->size.x,
        startPos.y - transform2D->size.y,
        1.0f,
        1.0f
    };

    const GLuint lower  = static_cast<GLuint>(bindlessUID & 0xFFFFFFFF);
    const GLuint higher = static_cast<GLuint>(bindlessUID >> 32);
    glUniform2ui(4, lower, higher);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    App->GetOpenGLModule()->DrawArrays(GL_TRIANGLES, 0, 6);

    // glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

void ImageComponent::InitBuffers()
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

    bindlessUID = glGetTextureHandleARB(texture->GetTextureID());
    glMakeTextureHandleResidentARB(bindlessUID);
}

void ImageComponent::ClearBuffers() const
{
    if (vbo != 0) glDeleteBuffers(1, &vbo);
    if (vao != 0) glDeleteVertexArrays(1, &vao);

    glMakeTextureHandleNonResidentARB(bindlessUID);
    App->GetResourcesModule()->ReleaseResource(texture);
}

void ImageComponent::ChangeTexture(const UID textureUID)
{
    if (textureUID == INVALID_UID || (texture != nullptr && texture->GetUID() == textureUID)) return;

    // Change resource texture
    Resource* newTexture = App->GetResourcesModule()->RequestResource(textureUID);
    if (newTexture != nullptr)
    {
        glMakeTextureHandleNonResidentARB(bindlessUID);
        App->GetResourcesModule()->ReleaseResource(texture);

        texture     = static_cast<ResourceTexture*>(newTexture);
        bindlessUID = glGetTextureHandleARB(texture->GetTextureID());
        glMakeTextureHandleResidentARB(bindlessUID);
    }
}

void ImageComponent::MatchParentSize()
{
    if (transform2D == nullptr) return;

    const UID parentUID  = parent->GetParent();
    GameObject* parentGO = App->GetSceneModule()->GetScene()->GetGameObjectByUID(parentUID);

    // Check if parent has transform 2D
    if (Transform2DComponent* parentT2D = parentGO->GetComponent<Transform2DComponent*>())
    {
        transform2D->size     = parentT2D->size;
        transform2D->position = float2(0, 0);
        return;
    }

    // Check if parent is a canvas
    if (CanvasComponent* canvas = parentGO->GetComponent<CanvasComponent*>())
    {
        transform2D->size     = float2(canvas->GetWidth(), canvas->GetHeight());
        transform2D->position = float2(0, 0);
        return;
    }
}