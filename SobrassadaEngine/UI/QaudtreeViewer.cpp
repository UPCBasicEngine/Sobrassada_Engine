#include "QaudtreeViewer.h"

#include "Application.h"
#include "CameraModule.h"
#include "Framebuffer.h"
#include "Globals.h"
#include "ShaderModule.h"
#include "WindowModule.h"

#include "SDL.h"
#include "glew.h"
#include "imgui.h"

QaudtreeViewer::QaudtreeViewer()
{
    program     = App->GetShaderModule()->GetProgram("./Test/basicVertexShader.vs", "./Test/basicFragmentShader.fs");
    framebuffer = new Framebuffer(SCREEN_WIDTH, SCREEN_HEIGHT, true);
    // Triangle
    /*float vtx_data[] = {
        -1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
    };*/

    // Lines
    float vtx_data[] = {-1.0f, -1.0f, 0.f, 1.0f, -1.0f, 0.f};

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vtx_data), vtx_data, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // TODO: USE CAMERA COMPONENT / CLASS TO CREATE A ORTHOGONAL CAMERA FRON RENDERING THE QUADTREE
    camera.type               = FrustumType::OrthographicFrustum;
    camera.pos                = float3(0, 0, 5);
    camera.front              = -float3::unitZ;
    camera.up                 = float3::unitY;
    camera.nearPlaneDistance  = 0.1f;
    camera.farPlaneDistance   = 100.f;

    camera.orthographicHeight = (float)framebuffer->GetTextureHeight() / cameraSizeScaleFactor;
    camera.orthographicWidth  = (float)framebuffer->GetTextureWidth() / cameraSizeScaleFactor;

    viewMatrix                = camera.ViewMatrix();
    projectionMatrix          = camera.ProjectionMatrix();
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
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    float4x4 model;
    model = float4x4::identity;

    glUseProgram(program);
    glUniformMatrix4fv(0, 1, GL_TRUE, &projectionMatrix[0][0]);
    glUniformMatrix4fv(1, 1, GL_TRUE, &viewMatrix[0][0]);
    glUniformMatrix4fv(2, 1, GL_TRUE, &model[0][0]);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);

    // Triangle
    // glDrawArrays(GL_TRIANGLES, 0, 3);
    glDrawArrays(GL_LINES, 0, 2);

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
                ChangeCameraSize(windowSize.x, windowSize.y);
                framebuffer->Resize((int)windowSize.x, (int)windowSize.y);
            }
        }
    }
    ImGui::End();
}

void QaudtreeViewer::ChangeCameraSize(float width, float height) 
{
    camera.orthographicHeight = width / cameraSizeScaleFactor;
    camera.orthographicWidth  = height / cameraSizeScaleFactor;

    viewMatrix                = camera.ViewMatrix();
    projectionMatrix          = camera.ProjectionMatrix();
}
