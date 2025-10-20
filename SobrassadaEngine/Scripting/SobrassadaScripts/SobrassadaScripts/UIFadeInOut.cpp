#include "pch.h"

#include "UIFadeInOut.h"

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

UIFadeInOut::UIFadeInOut(GameObject* parent) : Script(parent)
{
    fields.push_back({"Start visible", InspectorField::FieldType::Bool, &startVisible});

    fields.push_back({"Fade in duration", InspectorField::FieldType::Float, &fadeInDuration, 0.0f, 10.0f});
    fields.push_back({"Fade in opacity", InspectorField::FieldType::Float, &fadeInOpacity, 0.0f, 1.0f});
    fields.push_back({"Fade out duration", InspectorField::FieldType::Float, &fadeOutDuration, 0.0f, 10.0f});
    fields.push_back({"Fade out opacity", InspectorField::FieldType::Float, &fadeOutOpacity, 0.0f, 1.0f});

    fields.push_back({InspectorField::FieldType::Text, (void*)"Automation parameters"});
    fields.push_back({"Automatic fade in", InspectorField::FieldType::Bool, &fadeInAuto});
    fields.push_back({"Fade in start", InspectorField::FieldType::Float, &automaticFadeInStart, 0.0f, 100.0f});
    fields.push_back({"Automatic fade out", InspectorField::FieldType::Bool, &fadeOutAuto});
    fields.push_back({"Fade out start", InspectorField::FieldType::Float, &automaticFadeOutStart, 0.0f, 100.0f});
}

UIFadeInOut::~UIFadeInOut()
{
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
}

bool UIFadeInOut::Init()
{
    // This init is being called twice
    shaderProgram = AppEngine->GetShaderModule()->RequestShaderProgram(
        "./EngineDefaults/Shader/Vertex/UIWidgetVertex.glsl",
        "./EngineDefaults/Shader/Custom/Fragment/UIFadeInOut_Fragment.glsl"
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

    imageComp->SetEnabled(false);
    isVisible = startVisible;

    return true;
}

void UIFadeInOut::Update(float deltaTime)
{
}

void UIFadeInOut::Render(float deltaTime, CameraComponent* cameraComp)
{
    if (!shaderProgram || !imageComp) return;

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

    timer += deltaTime;
    if (isFadingIn && timer - fadeInStart > fadeInDuration) isFadingIn = false;
    if (isFadingOut && timer - fadeOutStart > fadeOutDuration) isFadingOut = false;

    if (!hasFadedIn && fadeInAuto && timer > automaticFadeInStart) FadeIn();
    if (!hasFadedOut && fadeOutAuto && timer > automaticFadeOutStart) FadeOut();

    glUniform1f(4, timer);

    glUniform1i(5, isFadingIn);
    glUniform1f(6, fadeInStart);
    glUniform1f(7, fadeInDuration);
    glUniform1f(8, fadeInOpacity);

    glUniform1i(9, isFadingOut);
    glUniform1f(10, fadeOutStart);
    glUniform1f(11, fadeOutDuration);
    glUniform1f(12, fadeOutOpacity);

    glUniform1i(13, isVisible);

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

    const UID bindlessUID = imageComp->GetTextureUID();
    const GLuint lower    = static_cast<GLuint>(bindlessUID & 0xFFFFFFFF);
    const GLuint higher   = static_cast<GLuint>(bindlessUID >> 32);
    glUniform2ui(3, lower, higher);

    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    AppEngine->GetOpenGLModule()->DrawArrays(GL_TRIANGLES, 0, 6);

    glDisable(GL_BLEND);
}

void UIFadeInOut::Reset()
{
    timer        = 0.0f;
    isVisible    = startVisible;

    isFadingIn   = false;
    fadeInStart  = 0.0f;

    isFadingOut  = false;
    fadeOutStart = 0.0f;

    hasFadedIn   = false;
    hasFadedOut  = false;
}

void UIFadeInOut::FadeIn()
{
    isVisible   = true;
    isFadingIn  = true;
    fadeInStart = timer;

    hasFadedIn  = true;
}

void UIFadeInOut::FadeOut()
{
    isVisible    = false;
    isFadingOut  = true;
    fadeOutStart = timer;

    hasFadedOut  = true;
}
