#include "QaudtreeViewer.h"

#include "Application.h"
#include "Framebuffer.h"
#include "Globals.h"
#include "WindowModule.h"
#include "ShaderModule.h"
#include "CameraModule.h"

#include "SDL.h"
#include "glew.h"
#include "imgui.h"

QaudtreeViewer::QaudtreeViewer()
{
    // TODO: USE CAMERA COMPONENT / CLASS TO CREATE A ORTHOGONAL CAMERA FRON RENDERING THE QUADTREE
    program     = App->GetShaderModule()->GetProgram("./Test/basicVertexShader.vs", "./Test/basicFragmentShader.fs");
    framebuffer = new Framebuffer(SCREEN_WIDTH, SCREEN_HEIGHT, true);
    float vtx_data[] = {
        -1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
    };

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vtx_data), vtx_data, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

QaudtreeViewer::~QaudtreeViewer()
{
    delete framebuffer;
    glDeleteBuffers(1, &vbo);
}

void QaudtreeViewer::Render(bool &renderBoolean)
{
    framebuffer->CheckResize();
    framebuffer->Bind();

    // Clear current viewport
    glViewport(0, 0, framebuffer->GetTextureWidth(), framebuffer->GetTextureHeight());
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    float4x4 model, view, proj;
    proj  = App->GetCameraModule()->GetProjectionMatrix();
    view  = App->GetCameraModule()->GetViewMatrix();
    model = float4x4::identity;

    glUseProgram(program);
    glUniformMatrix4fv(0, 1, GL_TRUE, &proj[0][0]);
    glUniformMatrix4fv(1, 1, GL_TRUE, &view[0][0]);
    glUniformMatrix4fv(2, 1, GL_TRUE, &model[0][0]);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);

    glDrawArrays(GL_TRIANGLES, 0, 3);

    // Unbind everything to not mess with other renders
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(0);

    framebuffer->Unbind();

    if (ImGui::Begin("Quadtree", &renderBoolean))
    {
        if (ImGui::BeginChild(
                "##SceneChild", ImVec2(0.f, 0.f), NULL,
                ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar
            ))
        {
            ImGui::SetCursorPos(ImVec2(0.f, 0.f));

            ImGui::Image(
                (ImTextureID)framebuffer->GetTextureID(),
                ImVec2((float)framebuffer->GetTextureWidth(), (float)framebuffer->GetTextureHeight()), ImVec2(0.f, 1.f),
                ImVec2(1.f, 0.f)
            );

            ImGui::EndChild();

            ImVec2 windowSize = ImGui::GetWindowSize();
            if (framebuffer->GetTextureWidth() != windowSize.x || framebuffer->GetTextureHeight() != windowSize.y)
            {
                float aspectRatio = windowSize.y / windowSize.x;
                framebuffer->Resize((int)windowSize.x, (int)windowSize.y);
            }
        }
    }
    ImGui::End();
}
