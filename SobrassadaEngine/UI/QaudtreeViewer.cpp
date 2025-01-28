#include "QaudtreeViewer.h"

#include "Application.h"
#include "Framebuffer.h"
#include "Globals.h"
#include "DebugDrawModule.h"

#include "SDL.h"
#include "glew.h"
#include "imgui.h"

QaudtreeViewer::QaudtreeViewer()
{
    framebuffer = new Framebuffer(SCREEN_WIDTH, SCREEN_HEIGHT, true);

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
}

void QaudtreeViewer::Render(bool &renderBoolean)
{
    framebuffer->CheckResize();
    framebuffer->Bind();

    // Clear current viewport
    glViewport(0, 0, framebuffer->GetTextureWidth(), framebuffer->GetTextureHeight());
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    App->GetDebugDreawModule()->RenderLines(viewMatrix, projectionMatrix, framebuffer->GetTextureWidth(), framebuffer->GetTextureHeight());

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
