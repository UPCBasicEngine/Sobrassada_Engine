#include "QuadtreeViewer.h"

#include "Application.h"
#include "DebugDrawModule.h"
#include "Framebuffer.h"
#include "Globals.h"
#include "Quadtree.h"
#include "CameraModule.h"

#include "MockGameObject.h"

#include "Algorithm/Random/LCG.h"
#include "SDL.h"
#include "glew.h"
#include "imgui.h"

QuadtreeViewer::QuadtreeViewer()
{
    int quadtreeSize          = 20;
    framebuffer               = new Framebuffer(SCREEN_WIDTH, SCREEN_HEIGHT, true);
    quadtree                  = new Quadtree(float2(0, 0), (float)quadtreeSize, 2);

    // TODO: USE CAMERA COMPONENT / CLASS TO CREATE A ORTHOGONAL CAMERA FRON RENDERING THE QUADTREE
    camera.type               = FrustumType::OrthographicFrustum;
    camera.pos                = float3(0, 0, 100);
    camera.front              = -float3::unitZ;
    camera.up                 = float3::unitY;
    camera.nearPlaneDistance  = 0.1f;
    camera.farPlaneDistance   = 200.f;

    float aspectRatio         = (float)framebuffer->GetTextureHeight() / (float)framebuffer->GetTextureWidth();

    camera.orthographicWidth  = (float)framebuffer->GetTextureWidth();
    camera.orthographicHeight = (float)framebuffer->GetTextureHeight();

    viewMatrix                = camera.ViewMatrix();
    projectionMatrix          = camera.ProjectionMatrix();

    // TODO: REMOVE, JUST FOR TESTING QUADTREE GENERATION
    math::LCG randomGenerator;
    int samplePoints       = 10;
    float objectsSize      = 1;
    float halfObjectsSize  = objectsSize / 2.f;
    float halfQuadtreeSize = quadtreeSize / 2.f;

    for (int i = 0; i < samplePoints; ++i)
    {
        float xCoord    = randomGenerator.Float(-(halfQuadtreeSize - objectsSize), (halfQuadtreeSize - objectsSize));
        float yCoord    = randomGenerator.Float(-(halfQuadtreeSize - objectsSize), (halfQuadtreeSize - objectsSize));
        float zCoord    = randomGenerator.Float(-(halfQuadtreeSize - objectsSize), (halfQuadtreeSize - objectsSize));

        float3 minPoint = float3(xCoord - halfObjectsSize, yCoord - halfObjectsSize, zCoord - halfObjectsSize);
        float3 maxPoint = float3(xCoord + halfObjectsSize, yCoord + halfObjectsSize, zCoord + halfObjectsSize);

        MockGameObject *newGameObject = new MockGameObject(minPoint, maxPoint);
        gameObjects.push_back(newGameObject);

        quadtree->InsertElement(gameObjects[i]);
    }
}

QuadtreeViewer::~QuadtreeViewer()
{
    delete framebuffer;
    delete quadtree;

    for (auto &gameObject : gameObjects)
    {
        delete gameObject;
    }

    gameObjects.clear();
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
    std::vector<float4> gameObjectLines;
    quadtree->GetDrawLines(drawLines, elementLines);

    AABB queryArea = AABB(float3(-10, -10, -10), float3(10, 10, 10));
    std::vector<const MockGameObject *> retrievedElements;
    quadtree->QueryElements(queryArea, retrievedElements);

    CreateQueryAreaLines(queryArea, queryAreaLines);
    CreateGameObjectsAreaLines(gameObjectLines);

    // RENDER QUADTREE
    App->GetDebugDrawModule()->Render2DLines(drawLines, float3(1.f, 0.f, 0.f), 0.f);

    // RENDER ELEMENTS INSIDE THE QUADTREE
    App->GetDebugDrawModule()->Render2DLines(elementLines, float3(0.f, 0.f, 1.f), 1.f);

    // RENDER QUERY AREA
    App->GetDebugDrawModule()->Render2DLines(queryAreaLines, float3(0.f, 1.f, 0.f), 2.f);

    // RENDER ALL ELEMENTS TO CHECK IF SOME DO NOT GET INSERTED
    App->GetDebugDrawModule()->Render2DLines(gameObjectLines, float3(1.f, 1.f, 1.f), -1.f);

    App->GetDebugDrawModule()->Draw(
        viewMatrix, projectionMatrix, framebuffer->GetTextureWidth(), framebuffer->GetTextureHeight()
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
    camera.orthographicHeight = height;
    camera.orthographicWidth  = width;

    viewMatrix                = camera.ViewMatrix();
    projectionMatrix          = camera.ProjectionMatrix();
}

void QuadtreeViewer::CreateQueryAreaLines(const AABB &queryArea, std::vector<float4> &queryAreaLines) const
{
    AABB2D area2D      = AABB2D(queryArea.minPoint.xz(), queryArea.maxPoint.xz());
    float2 center      = area2D.CenterPoint();
    queryAreaLines     = std::vector<float4>(4);

    float halfSizeX    = area2D.Width() / 2.f;
    float halfSizeY    = area2D.Height() / 2.f;

    float2 topLeft     = float2(center.x - halfSizeX, center.y + halfSizeY);
    float2 topRight    = float2(center.x + halfSizeX, center.y + halfSizeY);
    float2 bottomLeft  = float2(center.x - halfSizeX, center.y - halfSizeY);
    float2 bottomRight = float2(center.x + halfSizeX, center.y - halfSizeY);

    queryAreaLines[0]  = float4(topLeft.x, topLeft.y, topRight.x, topRight.y);
    queryAreaLines[1]  = float4(topRight.x, topRight.y, bottomRight.x, bottomRight.y);
    queryAreaLines[2]  = float4(bottomLeft.x, bottomLeft.y, bottomRight.x, bottomRight.y);
    queryAreaLines[3]  = float4(topLeft.x, topLeft.y, bottomLeft.x, bottomLeft.y);
}

void QuadtreeViewer::CreateGameObjectsAreaLines(std::vector<float4> &elementsAreaLines) const
{
    elementsAreaLines = std::vector<float4>(gameObjects.size() * 4);
    int currentLine   = 0;
    for (const auto &element : gameObjects)
    {
        AABB2D area2D =
            AABB2D(element->GetWorldBoundingBox().minPoint.xz(), element->GetWorldBoundingBox().maxPoint.xz());

        float halfSizeX                  = area2D.Width() / 2.f;
        float halfSizeY                  = area2D.Height() / 2.f;

        float2 center                    = area2D.CenterPoint();
        float2 topLeft                   = float2(center.x - halfSizeX, center.y + halfSizeY);
        float2 topRight                  = float2(center.x + halfSizeX, center.y + halfSizeY);
        float2 bottomLeft                = float2(center.x - halfSizeX, center.y - halfSizeY);
        float2 bottomRight               = float2(center.x + halfSizeX, center.y - halfSizeY);

        elementsAreaLines[currentLine++] = float4(topLeft.x, topLeft.y, topRight.x, topRight.y);
        elementsAreaLines[currentLine++] = float4(topRight.x, topRight.y, bottomRight.x, bottomRight.y);
        elementsAreaLines[currentLine++] = float4(bottomLeft.x, bottomLeft.y, bottomRight.x, bottomRight.y);
        elementsAreaLines[currentLine++] = float4(topLeft.x, topLeft.y, bottomLeft.x, bottomLeft.y);
    }
}
