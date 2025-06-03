#pragma once

#include "Module.h"
#include "Globals.h"

#include "Math/float4x4.h"
#include <bitset>
#include <list>

class DDRenderInterfaceCoreGL;
class dtNavMeshQuery;
class dtNavMesh;
class Camera;
class btVector3;

enum class DebugOptions : uint8_t
{
    RENDER_LIGTHS = 0,
    RENDER_WIREFRAME,
    RENDER_AABB,
    RENDER_OBB,
    RENDER_STATICTREE,
    RENDER_DYNAMICTREE,
    RENDER_CAMERA_RAY,
    RENDER_NAVMESH,
    RENDER_PHYSICS_WORLD,
    RENDER_GBUFFERS,
    RENDER_DEPTH,
    RENDER_NAVMESH_MESHES,
    RENDER_SPLINES,
    RENDER_DEBUG_VISUALS
};

enum DrawNavMeshFlags
{
    DRAWNAVMESH_OFFMESHCONS = 0x01,
    DRAWNAVMESH_CLOSEDLIST  = 0x02,
    DRAWNAVMESH_COLOR_TILES = 0x04
};

constexpr const char* DebugStrings[] = {"Render Lights", "Render Wireframe", "AABB",    "OBB",           "Static Tree",
                                        "Dynamic Tree",  "Camera Ray",       "Navmesh", "Physics World", "GBuffers",
                                        "Depth",         "Navmesh Meshes",   "Splines", "Debug Visuals"};

class DebugDrawModule : public Module
{
  public:
    DebugDrawModule();
    ~DebugDrawModule() override;

    bool Init() override;
    update_status Render(float deltaTime) override;
    bool ShutDown() override;

    void Draw();
    void Draw(const float4x4& view, const float4x4& proj, unsigned width, unsigned height);
    void Render2DLines(const std::vector<float4>& lines, const float3& color, float depth);
    void RenderLines(const std::vector<LineSegment>& lines, const float3& color);
    void DrawLineSegment(const LineSegment& line, const float3& color);
    void DrawLine(
        const float3& origin, const float3& direction, const float distance, const float3& color,
        bool enableDepth = true
    );
    void DrawLine(const btVector3& from, const btVector3& to, const btVector3& color);

    void DrawFrustrum(float4x4 frustumProj, float4x4 frustumView);
    void DrawCircle(const float3& center, const float3& upVector, const float3& color, const float radius);
    void DrawSphere(const float3& center, const float3& color, const float radius);
    void DrawAxisTriad(const float4x4& transform, bool depthEnabled = true);
    void DrawCross(const float3& center, const float length);
    void DrawPoint(const float3& center, const float size);
    void DrawCone(const float3& center, const float3& dir, const float baseRadius, const float apexRadius);
    SOBRASADA_API_ENGINE void Draw3DText(const btVector3& location, const char* textString);
    SOBRASADA_API_ENGINE void Draw2DText(const char* textString, const float3& location, const float3& color = float3::zero, float scale = 1.0f);
    void DrawContactPoint(
        const btVector3& PointOnB, const btVector3& normalOnB, float distance, int lifeTime, const btVector3& color
    );

    void DrawNavMesh(const dtNavMesh* navMesh, const dtNavMeshQuery* navQuery, unsigned char flags);

    void FlipDebugOptionValue(int position) { debugOptionValues.flip(position); }

    const std::bitset<(sizeof(DebugStrings) / sizeof(char*))>& GetDebugOptionValues() { return debugOptionValues; }

    bool GetDebugOptionValue(int position) { return debugOptionValues[position]; }

  private:
    void HandleDebugRenderOptions();

  private:
    static DDRenderInterfaceCoreGL* implementation;
    std::bitset<(sizeof(DebugStrings) / sizeof(char*))> debugOptionValues;
};
