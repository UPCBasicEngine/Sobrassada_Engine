#include "LightsConfig.h"

#include "Application.h"
#include "CameraModule.h"
#include "OpenGLModule.h"
#include "ShaderModule.h"
#include "TextureImporter.h"
#include "imgui.h"

#include "../Scene/Components/Standalone/Lights/DirectionalLight.h"
#include "../Scene/Components/Standalone/Lights/PointLight.h"
#include "../Scene/Components/Standalone/Lights/SpotLight.h"
#include "glew.h"
#include <cstddef>

LightsConfig::LightsConfig()
{
    skyboxTexture    = 0;
    skyboxVao        = 0;
    skyboxProgram    = 0;
    ambientColor     = float3(1.0f, 1.0f, 1.0f);
    ambientIntensity = 0.2f;

    // pointLights.push_back(PointLight(float3(-2, 0, 0), 1));
    // pointLights.push_back(PointLight(float3(2, 0, 0), 1));
    // pointLights.push_back(PointLight(float3(0, 1, -2), 1));
    //
    // spotLights.push_back(SpotLight(float3(0, 3, 0), -float3::unitY));
    // spotLights.push_back(SpotLight(float3(-4, 1, 0), float3::unitX));
    // spotLights.push_back(SpotLight(float3(0, 1, 4), -float3::unitZ));

    // directionalLight = new DirectionalLight();
}

LightsConfig::~LightsConfig()
{
    glDeleteBuffers(1, &ambientBufferId);
    glDeleteBuffers(1, &directionalBufferId);
    glDeleteBuffers(1, &pointBufferId);
    glDeleteBuffers(1, &spotBufferId);
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
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

    glBindVertexArray(0);

    skyboxTexture = LoadSkyboxTexture("Test/cubemap.dds");

    // Load the skybox shaders
    skyboxProgram = App->GetShaderModule()->GetProgram("Test/skyboxVertex.glsl", "Test/skyboxFragment.glsl");
}

void LightsConfig::RenderSkybox() const
{
    App->GetOpenGLModule()->SetDepthFunc(false);

    auto projection = App->GetCameraModule()->GetProjectionMatrix();
    auto view       = App->GetCameraModule()->GetViewMatrix();

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

unsigned int LightsConfig::LoadSkyboxTexture(const char* filename) const
{
    std::string stringPath = std::string(filename);
    UID skyboxUID          = TextureImporter::Import(stringPath.c_str());
    // TextureImporter::LoadTexture(skyboxUID);
    return 0;
}

void LightsConfig::EditorParams()
{
    ImGui::Begin("Lights Config");

    ImGui::SeparatorText("Ambient light");
    ImGui::SliderFloat3("Ambient color", &ambientColor[0], 0, 1);
    ImGui::SliderFloat("Ambient intensity", &ambientIntensity, 0, 1);

    ImGui::End();

    int index = 0;
    if (directionalLight)
    {
        directionalLight->EditorParams(0);
    }
    for (SpotLight& spot : spotLights)
    {
        spot.EditorParams(index);
        spot.DrawGizmos();
        ++index;
    }
    index = 0;
    for (PointLight& point : pointLights)
    {
        point.EditorParams(index);
        point.DrawGizmos();
        ++index;
    }
}

void LightsConfig::InitLightBuffers()
{
    glGenBuffers(1, &ambientBufferId);

    // Buffer for the Directional Light
    glGenBuffers(1, &directionalBufferId);
    glBindBuffer(GL_UNIFORM_BUFFER, directionalBufferId);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(Lights::DirectionalLightShaderData), nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 7, directionalBufferId);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    glGenBuffers(1, &pointBufferId);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, pointBufferId);
    size_t bufferSize = sizeof(Lights::PointLightShaderData) * pointLights.size() + 16;
    glBufferData(GL_SHADER_STORAGE_BUFFER, bufferSize, nullptr, GL_STATIC_DRAW);

    glGenBuffers(1, &spotBufferId);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, spotBufferId);
    bufferSize =
        (sizeof(Lights::SpotLightShaderData) + 12) * spotLights.size() + 16; // 12 bytes offset between spotlights
    glBufferData(GL_SHADER_STORAGE_BUFFER, bufferSize, nullptr, GL_STATIC_DRAW);
}

void LightsConfig::SetLightsShaderData() const
{
    // Ambient light
    Lights::AmbientLightShaderData ambient = Lights::AmbientLightShaderData(float4(ambientColor, ambientIntensity));

    glBindBuffer(GL_UNIFORM_BUFFER, ambientBufferId);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(ambient), &ambient, GL_STATIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 4, ambientBufferId);

    SetDirectionalLightShaderData();
    SetPointLightsShaderData();
    SetSpotLightsShaderData();
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

void LightsConfig::SetPointLightsShaderData() const
{
    std::vector<Lights::PointLightShaderData> points;
    for (int i = 0; i < pointLights.size(); ++i)
    {
        // Fill struct data
        points.emplace_back(Lights::PointLightShaderData(
            float4(pointLights[i].GetGlobalTransform().position, pointLights[i].GetRange()),
            float4(pointLights[i].GetColor(), pointLights[i].GetIntensity())
        ));
    }

    // This only works whith a constant number of lights. If a new light is added, the buffer must be resized
    int count = static_cast<int>(points.size());
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, pointBufferId);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(int), &count);
    int offset = 16; // Byte start offset for the point light array in the SSBO
    for (const Lights::PointLightShaderData& light : points)
    {
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, sizeof(Lights::PointLightShaderData), &light);
        offset += sizeof(Lights::PointLightShaderData);
    }
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, pointBufferId);
}

void LightsConfig::SetSpotLightsShaderData() const
{
    // This could probably include only lights that changed
    std::vector<Lights::SpotLightShaderData> spots;
    for (int i = 0; i < spotLights.size(); ++i)
    {
        // Fill struct data
        spots.emplace_back(Lights::SpotLightShaderData(
            float4(spotLights[i].GetGlobalTransform().position, spotLights[i].GetRange()),
            float4(spotLights[i].GetColor(), spotLights[i].GetIntensity()), float3(spotLights[i].GetDirection()),
            spotLights[i].GetInnerAngle(), spotLights[i].GetOuterAngle()
        ));
    }

    // This only works whith a constant number of lights. If a new light is added, the buffer must be resized
    int count = static_cast<int>(spots.size());
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, spotBufferId);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(int), &count);
    int offset = 16; // Byte start offset for the point light array in the SSBO
    for (const Lights::SpotLightShaderData& light : spots)
    {
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, sizeof(Lights::SpotLightShaderData), &light);
        offset += sizeof(Lights::SpotLightShaderData) + 12;
    }
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, spotBufferId);
}

void LightsConfig::AddDirectionalLight()
{
}
void LightsConfig::RemoveDirectionalLight()
{
}
void LightsConfig::AddPointLight()
{
}
void LightsConfig::AddSpotLight()
{
}
void LightsConfig::RemovePointLight()
{
}
void LightsConfig::RemoveSpotLight()
{
}