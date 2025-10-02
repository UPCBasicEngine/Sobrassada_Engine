#include "pch.h"

#include "UISpritesheet.h"

#include "Application.h"
#include "CameraModule.h"
#include "Components/CameraComponent.h"
#include "Components/Standalone/UI/ImageComponent.h"
#include "Components/Standalone/UI/Transform2DComponent.h"
#include "GBuffer.h"
#include "GameObject.h"
#include "GeometryBatch.h"
#include "Mesh.h"
#include "OpenGLModule.h"
#include "ResourceMaterial.h"
#include "ResourceMesh.h"
#include "ResourcesModule.h"
#include "Scene.h"
#include "SceneModule.h"
#include "ShaderModule.h"
#include "ShaderScriptComponent.h"

#include "glew.h"

UISpritesheet::UISpritesheet(GameObject* parent) : Script(parent)
{
    fields.push_back({"Disable default image", InspectorField::FieldType::Bool, &disableDefaultImage});
    fields.push_back({"Cell width", InspectorField::FieldType::Float, &cellWidth, 0.f, 10000.f});
    fields.push_back({"Cell height", InspectorField::FieldType::Float, &cellHeight, 0.f, 10000.f});
    fields.push_back({"Update Rate", InspectorField::FieldType::Float, &updateRate, 0.0f, 1.0f});
    fields.push_back({"Row major", InspectorField::FieldType::Bool, &isRowMajor});
    fields.push_back({"Is One Shot", InspectorField::FieldType::Bool, &isOneShot});
    fields.push_back({"Is Fade Out", InspectorField::FieldType::Bool, &isFadeOut});
    fields.push_back({"Fade Out Duration", InspectorField::FieldType::Float, &fadeOutDuration, 0.0f, 10.0f});
    fields.push_back({"Texture", InspectorField::FieldType::Resource, &spritesheetUID});
}

UISpritesheet::~UISpritesheet()
{
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);

    glMakeTextureHandleNonResidentARB(spritesheetBindlessUID);
    AppEngine->GetResourcesModule()->ReleaseResource(spritesheet);
}

bool UISpritesheet::Init()
{
    // This init is being called twice
    shaderProgram = AppEngine->GetShaderModule()->RequestShaderProgram(
        "./EngineDefaults/Shader/Custom/Vertex/UISpritesheet_Vertex.glsl",
        "./EngineDefaults/Shader/Custom/Fragment/UISpritesheet_Fragment.glsl"
    );

    imageComp = parent->GetComponent<ImageComponent*>();

    if (!imageComp) return true;

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

    if (disableDefaultImage) imageComp->SetEnabled(false);

    // Init other texture
    if (spritesheetUID == INVALID_UID || spritesheet || spritesheetBindlessUID != INVALID_UID) return true;

    spritesheet = static_cast<ResourceTexture*>(AppEngine->GetResourcesModule()->RequestResource(spritesheetUID));
    if (spritesheet)
    {
        spritesheetBindlessUID = glGetTextureHandleARB(spritesheet->GetTextureID());
        glMakeTextureHandleResidentARB(spritesheetBindlessUID);

        uvRange.x = 0.0f;
        uvRange.y = cellWidth / static_cast<float>(spritesheet->GetTextureWidth());
        uvRange.z = 0.0f;
        uvRange.w = cellHeight / static_cast<float>(spritesheet->GetTextureHeight());
    }

    return true;
}

void UISpritesheet::Update(float deltaTime)
{
}

void UISpritesheet::Render(float deltaTime, CameraComponent* cameraComp)
{
    if (!shaderProgram || !imageComp) return;

    UpdateSprite(deltaTime);

    // Custom UVs in UI only work in screen space (for now)
    Transform2DComponent* transform2D = imageComp->GetTransform2D();
    float3 startPos                   = float3(transform2D->GetRenderingPosition(), 0);

    float4x4 view                     = float4x4::identity;
    float4x4 proj                     = float4x4::D3DOrthoProjLH(
        -1, 1, transform2D->GetParentCanvas()->GetWidth(), transform2D->GetParentCanvas()->GetHeight()
    );

    glUseProgram(shaderProgram);

    glUniformMatrix4fv(0, 1, GL_TRUE, parent->GetGlobalTransform().ptr());
    glUniformMatrix4fv(1, 1, GL_TRUE, view.ptr());
    glUniformMatrix4fv(2, 1, GL_TRUE, proj.ptr());

    glUniform3fv(3, 1, imageComp->GetColor().ptr());

    timer += deltaTime;
    glUniform4fv(3, 1, uvRange.ptr());
    glUniform1f(5, timer);
    glUniform1f(6, fadeOutTime);
    glUniform1f(7, fadeOutDuration);

    glBindVertexArray(vao);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    // Basic quad
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

    GLuint lower  = static_cast<GLuint>(spritesheetBindlessUID & 0xFFFFFFFF);
    GLuint higher = static_cast<GLuint>(spritesheetBindlessUID >> 32);
    glUniform2ui(4, lower, higher);

    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    AppEngine->GetOpenGLModule()->DrawArrays(GL_TRIANGLES, 0, 6);

    glDisable(GL_BLEND);
}

void UISpritesheet::Reset()
{
    uvRange.x   = 0.0f;
    uvRange.y   = cellWidth / static_cast<float>(spritesheet->GetTextureWidth());
    uvRange.z   = 0.0f;
    uvRange.w   = cellHeight / static_cast<float>(spritesheet->GetTextureHeight());

    fadeOutTime = 0.0f;
}

void UISpritesheet::UpdateSprite(float deltaTime)
{
    timer += deltaTime;
    if (timer < updateRate) return;

    if (isFadeOut && uvRange.y >= 1.0f && uvRange.w >= 1.0f)
    {
        if (fadeOutTime == 0.0f) fadeOutTime = timer;
        if (timer - fadeOutTime >= fadeOutDuration) parent->GetComponent<ShaderScriptComponent*>()->SetEnabled(false);
        return;
    }

    if (isRowMajor)
    {
        if (uvRange.y >= 1.0f)
        {
            uvRange.x  = 0.0f;
            uvRange.y  = cellWidth / static_cast<float>(spritesheet->GetTextureWidth());

            uvRange.z += cellHeight / static_cast<float>(spritesheet->GetTextureHeight());
            uvRange.w += cellHeight / static_cast<float>(spritesheet->GetTextureHeight());
        }
        else
        {
            uvRange.x += cellWidth / static_cast<float>(spritesheet->GetTextureWidth());
            uvRange.y += cellWidth / static_cast<float>(spritesheet->GetTextureWidth());
        }
    }
    else
    {
        if (uvRange.w >= 1.0f)
        {
            uvRange.z  = 0.0f;
            uvRange.w  = cellHeight / static_cast<float>(spritesheet->GetTextureHeight());

            uvRange.x += cellWidth / static_cast<float>(spritesheet->GetTextureWidth());
            uvRange.y += cellWidth / static_cast<float>(spritesheet->GetTextureWidth());
        }
        else
        {
            uvRange.z += cellHeight / static_cast<float>(spritesheet->GetTextureHeight());
            uvRange.w += cellHeight / static_cast<float>(spritesheet->GetTextureHeight());
        }
    }
    timer = 0.0f;

    if (isOneShot && uvRange.y >= 1.0f && uvRange.w >= 1.0f)
    {
        parent->GetComponent<ShaderScriptComponent*>()->SetEnabled(false);
    }
}
