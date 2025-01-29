#include "QaudtreeViewer.h"

#include "Application.h"
#include "DebugDrawModule.h"
#include "Framebuffer.h"
#include "Globals.h"
#include "Quadtree.h"

#include "Algorithm/Random/LCG.h"
#include "SDL.h"
#include "glew.h"
#include "imgui.h"

QaudtreeViewer::QaudtreeViewer()
{
    framebuffer               = new Framebuffer(SCREEN_WIDTH, SCREEN_HEIGHT, true);
    quadtree                  = new Quadtree(float2(0, 0), 20, 2);

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

    // TODO: REMOVE, JUST FOR TESTING QUADTREE GENERATION
    math::LCG randomGenerator;
    int samplePoints = 3;

    for (int i = 0; i < samplePoints; ++i)
    {
        float xCoord    = randomGenerator.Float(-10, 10);
        float yCoord    = randomGenerator.Float(-10, 10);
        float4 newPoint = float4(xCoord, yCoord, 1.f, 1.f);

        quadtree->InsertElement(newPoint);
    }
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

    std::list<float4> drawLines;
    std::list<float4> elementLines;
    quadtree->GetDrawLines(drawLines, elementLines);

    App->GetDebugDreawModule()->RenderLines(
        viewMatrix, projectionMatrix, framebuffer->GetTextureWidth(), framebuffer->GetTextureHeight(), drawLines,
        elementLines
    );

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
