#include "LightsConfig.h"

#include "Application.h"
#include "TextureModuleTest.h"
#include "ShaderModule.h"
#include "CameraModule.h"
#include "OpenGLModule.h"
#include "imgui.h"

#include "../Scene/Components/PointLight.h"
#include "../Scene/Components/SpotLight.h"

#include "glew.h"

LightsConfig::LightsConfig()
{
    skyboxTexture    = 0;
    skyboxVao        = 0;
    skyboxProgram    = 0;
    ambientColor     = float3(1.0f, 1.0f, 1.0f);
    ambientIntensity = 0.2f;

    pointLights.push_back(PointLight(float3(-2, 0, 0), 1));
    pointLights.push_back(PointLight(float3(2, 0, 0), 1));
    pointLights.push_back(PointLight(float3(0, 1, -2), 1));

    spotLights.push_back(SpotLight(float3(0, 3, 0), -float3::unitY));
    spotLights.push_back(SpotLight(float3(-4, 1, 0), float3::unitX));
    spotLights.push_back(SpotLight(float3(0, 1, 4), -float3::unitZ));
}

LightsConfig::~LightsConfig() {}

void LightsConfig::InitSkybox()
{
    float skyboxVertices[] = {
        -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f,
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
        1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f
    };

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

    //Load the skybox shaders
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

void LightsConfig::EditorParams()
{ 
    ImGui::Begin("Lights Config");

    ImGui::SeparatorText("Ambient light");
    ImGui::SliderFloat3("Ambient color", &ambientColor[0], 0, 1);
    ImGui::SliderFloat("Ambient intensity", &ambientIntensity, 0, 1);

    ImGui::End();
}

void LightsConfig::SetLightsShaderData()
{
    // Ambient light
    Lights::AmbientLightShaderData ambient = Lights::AmbientLightShaderData(float4(ambientColor, ambientIntensity));
    unsigned int ambientId;
    glGenBuffers(1, &ambientId);
    glBindBuffer(GL_UNIFORM_BUFFER, ambientId);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(ambient), &ambient, GL_STATIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 4, ambientId);

    SetPointLightsShaderData();
    SetSpotLightsShaderData();
}

void LightsConfig::SetPointLightsShaderData()
{
    std::vector<Lights::PointLightShaderData> points;
    for (int i = 0; i < pointLights.size(); ++i)
    {
        pointLights[i].EditorParams(i);
        pointLights[i].DrawGizmos();

        // Fill struct data
        points.emplace_back(Lights::PointLightShaderData(
            float4(pointLights[i].GetPosition(), pointLights[i].GetRange()),
            float4(pointLights[i].GetColor(), pointLights[i].GetIntensity())
        ));
    }
    unsigned int pointId;
    glGenBuffers(1, &pointId);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, pointId);

    size_t dataSize = sizeof(Lights::PointLightShaderData);
    int bufferSize  = dataSize * points.size() + 16;
    glBufferData(GL_SHADER_STORAGE_BUFFER, bufferSize, nullptr, GL_STATIC_DRAW);
    int count = points.size();
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(int), &count);

    int offset = 16;
    for (const Lights::PointLightShaderData &light : points)
    {
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, dataSize, &light);
        offset += dataSize;
    }
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, pointId);
}

void LightsConfig::SetSpotLightsShaderData()
{
    std::vector<Lights::SpotLightShaderData> spots;
    for (int i = 0; i < spotLights.size(); ++i)
    {
        spotLights[i].EditorParams(i);
        spotLights[i].DrawGizmos();

        // Fill struct data
        spots.emplace_back(Lights::SpotLightShaderData(
            float4(spotLights[i].GetPosition(), spotLights[i].GetRange()),
            float4(spotLights[i].GetColor(), spotLights[i].GetIntensity()), float3(spotLights[i].GetDirection()),
            spotLights[i].GetInnerAngle(), spotLights[i].GetOuterAngle()
        ));
    }
    unsigned int spotId;
    glGenBuffers(1, &spotId);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, spotId);

    float dataSize   = sizeof(Lights::SpotLightShaderData);
    float bufferSize = (dataSize + 12) * spots.size() + 16; // 12 bytes offset between spotlights
    glBufferData(GL_SHADER_STORAGE_BUFFER, bufferSize, nullptr, GL_STATIC_DRAW);
    int count = spots.size();
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(int), &count);

    int offset = 16;
    for (const Lights::SpotLightShaderData &light : spots)
    {
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, dataSize, &light);
        offset += dataSize + 12;
    }
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, spotId);
}

unsigned int LightsConfig::LoadSkyboxTexture(const char* filename) const
{
    std::string stringPath         = std::string(filename);
    std::wstring widePath          = std::wstring(stringPath.begin(), stringPath.end());
    const wchar_t *wideTexturePath = widePath.c_str();
    return App->GetTextureModuleTest()->LoadCubemap(wideTexturePath);
    delete[] wideTexturePath;
}