#define NOMINMAX
#include "pch.h"

#include "DamageMask.h"

#include "Application.h"
#include "CameraModule.h"
#include "Components/CameraComponent.h"
#include "Components/Standalone/UI/ImageComponent.h"
#include "Components/Standalone/UI/Transform2DComponent.h"
#include "GBuffer.h"
#include "GameObject.h"
#include "GeometryBatch.h"
#include "Interpolation.h"
#include "Mesh.h"
#include "OpenGLModule.h"
#include "ResourceMaterial.h"
#include "ResourceMesh.h"
#include "Scene.h"
#include "SceneModule.h"
#include "ShaderModule.h"

#include "glew.h"
#include <algorithm>

DamageMask::DamageMask(GameObject* parent) : Script(parent)
{
    fields.push_back({"Noise Tiling", InspectorField::FieldType::Float, &noiseTiling, 0.0f, 10.0f});
    fields.push_back({"Noise Speed", InspectorField::FieldType::Float, &noiseSpeed, 0.0f, 1.0f});
}

DamageMask::~DamageMask()
{
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
}

bool DamageMask::Init()
{
    // This init is being called twice
    shaderProgram = AppEngine->GetShaderModule()->RequestShaderProgram(
        "./EngineDefaults/Shader/Vertex/UIWidgetVertex.glsl",
        "./EngineDefaults/Shader/Custom/Fragment/UI_DamageMask.glsl"
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

    return true;
}

void DamageMask::Update(float deltaTime)
{
}

void DamageMask::Render(float deltaTime, CameraComponent* cameraComp)
{
    // Custom UVs in UI only work in screen space (for now, won't change if no need)
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

    glUniform3fv(3, 1, imageComp->GetColor().ptr());

    const float elapsedTime  = std::min<float>(time - startTime, 0.25f);
    const float playerLife   = Interpolation::Lerp<float>(prevLife, nextLife, elapsedTime * 4.0f);

    time                    += deltaTime;
    float intensity          = std::max<float>(0.0f, 0.3f - (playerLife / 10.0f)) * 2.5f; // 2hp -> 2.5 | 1hp -> 5
    const float pulseSpeed   = std::max<float>(0.0f, 3.0f - nextLife)  * 4.0f;

    if (hitTimer > 0.0f)
    {
        const float t  = hitTimer > 0.2f ? 1.0f - ((hitTimer / 0.3f - 0.75f) * 4.0f) : hitTimer / 0.1f;
        hitTimer      -= deltaTime;
        intensity      = Interpolation::Lerp<float>(intensity, 0.6f, std::min<float>(1.0f, t));
    }

    glUniform1f(5, intensity);
    glUniform1f(6, pulseSpeed);
    glUniform1f(7, time);
    glUniform1f(8, noiseTiling);
    glUniform1f(9, noiseSpeed);

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

    UID bindlessUID     = imageComp->GetTextureUID();
    const GLuint lower  = static_cast<GLuint>(bindlessUID & 0xFFFFFFFF);
    const GLuint higher = static_cast<GLuint>(bindlessUID >> 32);
    glUniform2ui(4, lower, higher);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    AppEngine->GetOpenGLModule()->DrawArrays(GL_TRIANGLES, 0, 6);

    glDisable(GL_BLEND);
}

void DamageMask::Reset()
{
}
