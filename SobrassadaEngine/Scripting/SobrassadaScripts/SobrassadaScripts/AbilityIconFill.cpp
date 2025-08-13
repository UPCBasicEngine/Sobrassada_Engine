#include "pch.h"

#include "AbilityIconFill.h"

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
#include "Scene.h"
#include "SceneModule.h"
#include "ShaderModule.h"
#include "ResourcesModule.h"

#include "glew.h"

AbilityIconFill::AbilityIconFill(GameObject* parent) : Script(parent)
{
    fields.push_back({"Wave Amplitude", InspectorField::FieldType::Float, &waveAmplitude, 0.f, 1.0f});
    fields.push_back({"Wave Frequency", InspectorField::FieldType::Float, &waveFrequency, 0.f, 100.0f});
    fields.push_back({"Wave Speed", InspectorField::FieldType::Float, &waveSpeed, 0.f, 100.0f});
    fields.push_back({"Other Image", InspectorField::FieldType::Resource, &otherImageUID});
}

AbilityIconFill::~AbilityIconFill()
{
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);

    glMakeTextureHandleNonResidentARB(otherImageBindlessUID);
    AppEngine->GetResourcesModule()->ReleaseResource(otherImage);
}

bool AbilityIconFill::Init()
{
    // This init is being called twice
    shaderProgram = AppEngine->GetShaderModule()->RequestShaderProgram(
        "./EngineDefaults/Shader/Vertex/UIWidgetVertex.glsl",
        "./EngineDefaults/Shader/Custom/Fragment/UI_AbilitiesFill.glsl"
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

    // Init other texture
    if (otherImageUID == INVALID_UID || otherImage || otherImageBindlessUID != INVALID_UID) return true;

    otherImage =
        static_cast<ResourceTexture*>(AppEngine->GetResourcesModule()->RequestResource(otherImageUID) 
        );

    otherImageBindlessUID = glGetTextureHandleARB(otherImage->GetUID());
    glMakeTextureHandleResidentARB(otherImageBindlessUID);

    return true;
}

void AbilityIconFill::Update(float deltaTime)
{
}

void AbilityIconFill::Render(float deltaTime, CameraComponent* cameraComp)
{
    // Custom UVs in UI only work in screen space (for now)
    if (!shaderProgram || !imageComp) return;

    Transform2DComponent* transform2D = imageComp->GetTransform2D();
    float3 startPos                   = float3(transform2D->GetRenderingPosition(), 0);

    float4x4 view                     = float4x4::identity;
    float4x4 proj = float4x4::D3DOrthoProjLH(
        -1, 1, transform2D->GetParentCanvas()->GetWidth(), transform2D->GetParentCanvas()->GetHeight()
    );

    glUseProgram(shaderProgram);

    glUniformMatrix4fv(0, 1, GL_TRUE, parent->GetGlobalTransform().ptr());
    glUniformMatrix4fv(1, 1, GL_TRUE, view.ptr());
    glUniformMatrix4fv(2, 1, GL_TRUE, proj.ptr());

    glUniform3fv(3, 1, imageComp->GetColor().ptr());

    fillAmount = 0.3f;
    glUniform1f(6, fillAmount);

    glUniform1f(7, waveAmplitude);
    glUniform1f(8, waveFrequency);
    glUniform1f(9, waveSpeed);

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

    GLuint lower = static_cast<GLuint>(otherImageBindlessUID & 0xFFFFFFFF);
    GLuint higher = static_cast<GLuint>(otherImageBindlessUID >> 32);
    glUniform2ui(4, lower, higher);

    lower  = static_cast<GLuint>(imageComp->GetTextureUID() & 0xFFFFFFFF);
    higher = static_cast<GLuint>(imageComp->GetTextureUID() >> 32);
    glUniform2ui(5, lower, higher);   

    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    AppEngine->GetOpenGLModule()->DrawArrays(GL_TRIANGLES, 0, 6);

    glDisable(GL_BLEND);
}

void AbilityIconFill::Reset()
{
}

void AbilityIconFill::SetFillAmount(float newFill)
{
    fillAmount = newFill;
}
