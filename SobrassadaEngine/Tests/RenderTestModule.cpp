#include "RenderTestModule.h"

#include "Application.h"
#include "CameraModule.h"
#include "Framebuffer.h"
#include "OpenGLModule.h"
#include "SceneImporter.h"
#include "ShaderModule.h"
#include "TextureImporter.h"
#include "TextureModuleTest.h"
#include "WindowModule.h"
#include "LibraryModule.h"

#include "DirectXTex/DirectXTex.h"
#include "EngineModel.h"
#include "MathGeoLib.h"
#include "glew.h"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl2.h"

RenderTestModule::RenderTestModule() { currentLoadedModel = new EngineModel(); }

RenderTestModule::~RenderTestModule() {}

bool RenderTestModule::Init()
{
    // currentLoadedModel->Load("./Test/BakerHouse.gltf");

    // program = App->GetShaderModule()->GetProgram("./Test/basicVertexShader.vs", "./Test/basicFragmentShader.fs");
    program = App->GetShaderModule()->GetProgram("./Test/VertexShader.glsl", "./Test/FragmentShader.glsl");

    // Triangle without UV's
    /*float vtx_data[] = {
                -1.0f, -1.0f, 0.0f,
                1.0f, -1.0f, 0.0f,
                0.0f, 1.0f,
     * 0.0f,
        };*/

    // Texture loading
    DirectX::TexMetadata textureMetadata;

    SceneImporter::Import("./Test/baboon.ppm");
    std::string texturePath = "./Library/Textures/baboon.dds";

    baboonTexture           = App->GetTextureModuleTest()->LoadTexture(texturePath.c_str(), textureMetadata);

    // Quad with UV's
    float vtx_data[]        = {-1.0f, 1.0f,  0.0f, -1.0f, -1.0f, 0.0f, 1.0f,  -1.0f, 0.0f,

                               1.0f,  -1.0f, 0.0f, 1.0f,  1.0f,  0.0f, -1.0f, 1.0f,  0.0f,

                               0.0f,  0.0f,  0.0f, 1.0f,  1.0f,  1.0f,

                               1.0f,  1.0f,  1.0f, 0.0f,  0.0f,  0.0f};

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vtx_data), vtx_data, GL_STATIC_DRAW);

    return true;
}

update_status RenderTestModule::PreUpdate(float deltaTime) { return UPDATE_CONTINUE; }

update_status RenderTestModule::Render(float deltaTime)
{
    float4x4 model, view, proj;

    proj  = App->GetCameraModule()->GetProjectionMatrix();
    view  = App->GetCameraModule()->GetViewMatrix();
    model = float4x4::FromTRS(float3(2.0f, 0.0f, 0.0f), float4x4::RotateZ(pi / 4.0f), float3(2.0f, 1.0f, 1.0f));

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

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, baboonTexture);

    // glDrawArrays(GL_TRIANGLES, 0, 3);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    currentLoadedModel->Render(program, proj, view);

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