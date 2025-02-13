#include "LightsConfig.h"

#include "Application.h"
#include "CameraModule.h"
#include "OpenGLModule.h"
#include "ShaderModule.h"
#include "TextureModuleTest.h"
#include "imgui.h"

#include "../Scene/Components/DirectionalLight.h"

#include "glew.h"

LightsConfig::LightsConfig()
{
    skyboxTexture    = 0;
    skyboxVao        = 0;
    skyboxProgram    = 0;
    ambientColor     = float3(1.0f, 1.0f, 1.0f);
    ambientIntensity = 0.2f;

    directionalLight = new DirectionalLight();
}

LightsConfig::~LightsConfig()
{
    glDeleteBuffers(1, &directionalBufferId);
}

void LightsConfig::InitSkybox()
{
    float skyboxVertices[] = {-1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f,
                              1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f,

                              -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f,
                              -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,

                              1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,
                              1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f,

                              -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
                              1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,

                              -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,
                              1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f,

                              -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f,
                              1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f};

    // Generate VBO
    unsigned int vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);

    // Generato VAO
    glGenVertexArrays(1, &skyboxVao);
    glBindVertexArray(skyboxVao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);

    glBindVertexArray(0);

    skyboxTexture = LoadSkyboxTexture("Test/cubemap.dds");

    // Load the skybox shaders
    skyboxProgram = App->GetShaderModule()->GetProgram("Test/skyboxVertex.glsl", "Test/skyboxFragment.glsl");
}

void LightsConfig::RenderSkybox(float4x4 &projection, float4x4 &view) const
{
    App->GetOpenGLModule()->SetDepthFunc(false);

    glUseProgram(skyboxProgram);
    glUniformMatrix4fv(0, 1, GL_TRUE, &projection[0][0]);
    glUniformMatrix4fv(1, 1, GL_TRUE, &view[0][0]);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);

    glBindVertexArray(skyboxVao);
    glDrawArrays(GL_TRIANGLES, 0, 36);

    glBindVertexArray(0);

    App->GetOpenGLModule()->SetDepthFunc(true);
}

unsigned int LightsConfig::LoadSkyboxTexture(const char *filename) const
{
    std::string stringPath         = std::string(filename);
    std::wstring widePath          = std::wstring(stringPath.begin(), stringPath.end());
    const wchar_t *wideTexturePath = widePath.c_str();
    return App->GetTextureModuleTest()->LoadCubemap(wideTexturePath);
    delete[] wideTexturePath;
}

void LightsConfig::EditorParams()
{
   
    int index = 0;
    if (directionalLight)
    {
        directionalLight->EditorParams(0);
    }
}

void LightsConfig::InitLightBuffers()
{
    // Buffer for the Directional Light
    glGenBuffers(1, &directionalBufferId);
    glBindBuffer(GL_UNIFORM_BUFFER, directionalBufferId);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(Lights::DirectionalLightShaderData), nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 7, directionalBufferId);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void LightsConfig::SetLightsShaderData() const
{
    SetDirectionalLightShaderData();
}

void LightsConfig::SetDirectionalLightShaderData() const
{
    if (directionalLight)
    {
        Lights::DirectionalLightShaderData dirLightData(
            float3(directionalLight->GetDirection()),
            float4(directionalLight->GetColor(), directionalLight->GetIntensity())
        );

       glBindBuffer(GL_UNIFORM_BUFFER, directionalBufferId);
       glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(Lights::DirectionalLightShaderData), &dirLightData);
       glBindBuffer(GL_UNIFORM_BUFFER, 0);

    }
}

void LightsConfig::AddDirectionalLight()
{
    if (!directionalLight)
    {
        directionalLight = new DirectionalLight();
    }
}
void LightsConfig::RemoveDirectionalLight()
{
    if (directionalLight)
    {
        delete directionalLight;
        directionalLight = nullptr;
    }
}
