#include "RenderTestModule.h"

#include "Application.h"
#include "CameraModule.h"
#include "Components/ComponentMaterial.h"
#include "DebugDrawModule.h"
#include "Framebuffer.h"
#include "LightsConfig.h"
#include "OpenGLModule.h"
#include "Scene/Components/PointLight.h"
#include "Scene/Components/SpotLight.h"
#include "ShaderModule.h"
#include "TextureModuleTest.h"
#include "WindowModule.h"

#include "DirectXTex/DirectXTex.h"
#include "EngineModel.h"
#include "MathGeoLib.h"
#include "glew.h"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl2.h"

RenderTestModule::RenderTestModule()
{
    currentLoadedModel = new EngineModel();
    lightsConfig       = new LightsConfig();

    pointLights.push_back(PointLight(float3(-2, 0, 0), 1));
    pointLights.push_back(PointLight(float3(2, 0, 0), 1));
    spotLights.push_back(SpotLight());
}

RenderTestModule::~RenderTestModule() {}

bool RenderTestModule::Init()
{
    // currentLoadedModel->Load("./Test/BakerHouse.gltf");
    currentLoadedModel->Load("./Test/Robot.gltf");

    // program = App->GetShaderModule()->GetProgram("./Test/basicVertexShader.vs", "./Test/basicFragmentShader.fs");
    program = App->GetShaderModule()->GetProgram("./Test/VertexShader.glsl", "./Test/BRDFPhongFragmentShader.glsl");
    // program = App->GetShaderModule()->GetProgram("./Test/VertexShader.glsl", "./Test/FragmentShader.glsl");

    // Triangle without UV's
    /*float vtx_data[] = {
            -1.0f, -1.0f, 0.0f,
            1.0f, -1.0f, 0.0f,
            0.0f, 1.0f, 0.0f,
    };*/

    // Texture loading
    DirectX::TexMetadata textureMetadata;
    auto baboonPath  = L"./Test/baboon.ppm";
    baboonTexture    = App->GetTextureModuleTest()->LoadTexture(baboonPath, textureMetadata);

    // Quad with UV's
    float vtx_data[] = {-1.0f, 1.0f,  0.0f, -1.0f, -1.0f, 0.0f, 1.0f,  -1.0f, 0.0f,

                        1.0f,  -1.0f, 0.0f, 1.0f,  1.0f,  0.0f, -1.0f, 1.0f,  0.0f,

                        0.0f,  0.0f,  0.0f, 1.0f,  1.0f,  1.0f,

                        1.0f,  1.0f,  1.0f, 0.0f,  0.0f,  0.0f};

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vtx_data), vtx_data, GL_STATIC_DRAW);

    lightsConfig->InitSkybox();

    return true;
}

update_status RenderTestModule::PreUpdate(float deltaTime) { return UPDATE_CONTINUE; }

update_status RenderTestModule::Render(float deltaTime)
{
    float4x4 model, view, proj;

    proj  = App->GetCameraModule()->GetProjectionMatrix();
    view  = App->GetCameraModule()->GetViewMatrix();
    model = float4x4::FromTRS(float3(2.0f, 0.0f, 0.0f), float4x4::RotateZ(pi / 4.0f), float3(2.0f, 1.0f, 1.0f));

    // Draw the skybox before anything else
    lightsConfig->RenderSkybox(proj, view);

    glUseProgram(program);
    glUniformMatrix4fv(0, 1, GL_TRUE, &proj[0][0]);
    glUniformMatrix4fv(1, 1, GL_TRUE, &view[0][0]);
    glUniformMatrix4fv(2, 1, GL_TRUE, &model[0][0]);

    // Sending triangle vertices
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);

    // Sending texture coordiantes
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void *)(sizeof(float) * 3 * 6));

    // move this to materialcomponent to alter bia imgui
    float ambientIntensity = 0.0f;
    glUniform1f(glGetUniformLocation(program, "ambientIntensity"), ambientIntensity);
    float3 lightColor = float3::zero;
    glUniform3fv(glGetUniformLocation(program, "lightColor"), 1, &lightColor[0]);

    float3 cameraPos = App->GetCameraModule()->getPosition();
    glUniform3fv(glGetUniformLocation(program, "cameraPos"), 1, &cameraPos[0]);

    float3 diffFactor = float3(1.0f, 1.0f, 1.0f);
    glUniform3fv(glGetUniformLocation(program, "diffFactor"), 1, &diffFactor[0]);
    float3 specFactor = float3(1.0f, 1.0f, 1.0f);
    glUniform3fv(glGetUniformLocation(program, "specFactor"), 1, &specFactor[0]);
    if (ImGui::CollapsingHeader("Light"))
    {
        ImGui::SliderFloat3("Diffuse Color", &lightDir.x, -20.0f, 20.0f);
    }
    glUniform3fv(glGetUniformLocation(program, "lightDir"), 1, &lightDir[0]);
    materials = currentLoadedModel->GetMaterials();
    materials.at(0)->OnEditorUpdate();

    // BEGIN LIGHTS
    std::vector<Lights::PointLightData> lights;

    for (int i = 0; i < pointLights.size(); ++i)
    {
        // ImGui Menus
        if (i == 0) pointLights[i].EditorParams(i, true, false);
        else if (i = pointLights.size() - 1) pointLights[i].EditorParams(i, false, true);
        else pointLights[i].EditorParams(i, false, false);

        // Light gizmos
        pointLights[i].DrawGizmos();

        // Fill struct data
        float4 position = float4(pointLights[i].GetPosition(), pointLights[i].GetRange());
        float4 color    = float4(pointLights[i].GetColor(), pointLights[i].GetIntensity());
        lights.push_back(Lights::PointLightData(position, color));
    }

    unsigned int id;
    glGenBuffers(1, &id);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, id);

    // struct ShaderData
    //{
    //     float4 count;
    //     std::vector<Lights::PointLightData> points;
    // };
    //
    // ShaderData data;
    // data.count = float4(lights.size(), 0, 0, 0);
    // data.points = lights;

    // GLOG("Data size: %d", sizeof(Lights::PointLightData) * lights.size() + 32);

    int bufferSize = sizeof(Lights::PointLightData) * lights.size() + 16;
    glBufferData(GL_SHADER_STORAGE_BUFFER, bufferSize, nullptr, GL_DYNAMIC_DRAW);

    int count = lights.size();
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(int), &count);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 16, sizeof(Lights::PointLightData), &lights[0]);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 48, sizeof(Lights::PointLightData), &lights[1]);

    //float4 *ssboPtr  = reinterpret_cast<float4*>(glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY));
    //int i            = 0;
    //ssboPtr[i].x     = lights.size();
    //i               += 16;
    //ssboPtr[i]       = lights[0].position;
    //i               += 16;
    //ssboPtr[i]        = lights[0].color;
    //i               += 16;
    //ssboPtr[i]       = lights[1].position;
    //i               += 16;
    //ssboPtr[i]        = lights[1].color;
    //glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, id);

    spotLights[0].EditorParams();

    // Test point light
    glUniform3fv(3, 1, &pointLights[0].GetColor()[0]);
    glUniform3fv(4, 1, &pointLights[0].GetPosition()[0]);
    glUniform1f(5, pointLights[0].GetIntensity());
    glUniform1f(6, pointLights[0].GetRange());

    // Test spotlight
    glUniform3fv(7, 1, &spotLights[0].GetColor()[0]);
    glUniform3fv(8, 1, &spotLights[0].GetPosition()[0]);
    glUniform3fv(9, 1, &spotLights[0].GetDirection()[0]);
    glUniform1f(10, spotLights[0].GetIntensity());
    glUniform1f(11, spotLights[0].GetRange());
    glUniform1f(12, spotLights[0].GetInnerAngle());
    glUniform1f(13, spotLights[0].GetOuterAngle());

    spotLights[0].DrawGizmos();
    // END LIGHTS

    // glActiveTexture(GL_TEXTURE0);
    // glBindTexture(GL_TEXTURE_2D, baboonTexture);

    // glDrawArrays(GL_TRIANGLES, 0, 3);
    // glDrawArrays(GL_TRIANGLES, 0, 6);

    currentLoadedModel->Render(program, proj, view);

    int width  = 0;
    int height = 0;
    SDL_GetWindowSize(App->GetWindowModule()->window, &width, &height);

    // Draw the grid at last
    App->GetDebugDreawModule()->Draw(view, proj, width, height);

    // Unbinding frame buffer so ui gets rendered
    App->GetOpenGLModule()->GetFramebuffer()->Unbind();

    return UPDATE_CONTINUE;
}

bool RenderTestModule::ShutDown()
{
    App->GetShaderModule()->DeleteProgram(program);
    glDeleteBuffers(1, &vbo);
    glDeleteTextures(1, &baboonTexture);

    delete currentLoadedModel;
    delete lightsConfig;

    return true;
}

void RenderTestModule::RenderEditorViewport()
{
    ImGui::SetNextWindowSize(ImVec2(500, 500));
    ImGui::SetNextWindowPos(ImVec2(0, 50));
    ImGui::Begin("Scene");
    {
        ImGui::BeginChild("GameRender");

        float width  = ImGui::GetContentRegionAvail().x;
        float height = ImGui::GetContentRegionAvail().y;

        ImGui::Image((ImTextureID)0, ImGui::GetContentRegionAvail(), ImVec2(0, 1), ImVec2(1, 0));
    }
    ImGui::EndChild();
    ImGui::End();
}