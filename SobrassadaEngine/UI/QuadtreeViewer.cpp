#include "QuadtreeViewer.h"

#include "Application.h"
#include "DebugDrawModule.h"
#include "Framebuffer.h"
#include "Globals.h"
#include "Quadtree.h"

#include "Algorithm/Random/LCG.h"
#include "SDL.h"
#include "glew.h"
#include "imgui.h"

QuadtreeViewer::QuadtreeViewer()
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

    float aspectRatio         = (float)framebuffer->GetTextureHeight() / (float)framebuffer->GetTextureWidth();

    camera.orthographicWidth  = (float)framebuffer->GetTextureWidth();
    camera.orthographicHeight = (float)framebuffer->GetTextureHeight();

    viewMatrix                = camera.ViewMatrix();
    projectionMatrix          = camera.ProjectionMatrix();

    // TODO: REMOVE, JUST FOR TESTING QUADTREE GENERATION
    math::LCG randomGenerator;
    int samplePoints = 10;

    for (int i = 0; i < samplePoints; ++i)
    {
        float xCoord = randomGenerator.Float(-10, 10);
        float yCoord = randomGenerator.Float(-10, 10);
        Box newPoint = Box(xCoord, yCoord, 1.f, 1.f);

        quadtree->InsertElement(newPoint);
    }
}

QuadtreeViewer::~QuadtreeViewer()
{
    delete framebuffer;
    delete quadtree;
}

void QuadtreeViewer::Render(bool &renderBoolean)
{
    framebuffer->CheckResize();
    framebuffer->Bind();

    // Clear current viewport
    glViewport(0, 0, framebuffer->GetTextureWidth(), framebuffer->GetTextureHeight());
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    std::vector<float4> drawLines;
    std::vector<float4> elementLines;
    std::vector<float4> queryAreaLines;
    quadtree->GetDrawLines(drawLines, elementLines);

    Box queryArea = Box(5, 5, 5, 5);
    std::unordered_set<Box, BoxHash> rettrievedElements;
    quadtree->QueryElements(queryArea, rettrievedElements);

    CreateQueryAreaLines(queryArea, queryAreaLines);

    App->GetDebugDrawModule()->RenderQuadtreeViewportLines(
        viewMatrix, projectionMatrix, framebuffer->GetTextureWidth(), framebuffer->GetTextureHeight(), drawLines,
        elementLines, queryAreaLines
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

void QuadtreeViewer::ChangeCameraSize(float width, float height)
{
    camera.orthographicHeight = height / cameraSizeScaleFactor;
    camera.orthographicWidth = width / cameraSizeScaleFactor;

    viewMatrix                = camera.ViewMatrix();
    projectionMatrix          = camera.ProjectionMatrix();
}

void QuadtreeViewer::CreateQueryAreaLines(const Box &queryArea, std::vector<float4> &queryAreaLines) const
{
    queryAreaLines     = std::vector<float4>(4);

    float halfSizeX    = queryArea.sizeX / 2.f;
    float halfSizeY    = queryArea.sizeY / 2.f;

    float2 topLeft     = float2(queryArea.x - halfSizeX, queryArea.y + halfSizeY);
    float2 topRight    = float2(queryArea.x + halfSizeX, queryArea.y + halfSizeY);
    float2 bottomLeft  = float2(queryArea.x - halfSizeX, queryArea.y - halfSizeY);
    float2 bottomRight = float2(queryArea.x + halfSizeX, queryArea.y - halfSizeY);

    queryAreaLines[0]  = float4(topLeft.x, topLeft.y, topRight.x, topRight.y);
    queryAreaLines[1]  = float4(topRight.x, topRight.y, bottomRight.x, bottomRight.y);
    queryAreaLines[2]  = float4(bottomLeft.x, bottomLeft.y, bottomRight.x, bottomRight.y);
    queryAreaLines[3]  = float4(topLeft.x, topLeft.y, bottomLeft.x, bottomLeft.y);
}
