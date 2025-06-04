#include "NavMeshConfig.h"
#include "Application.h"
#include "FileSystem.h"
#include "LibraryModule.h"
#include "ProjectModule.h"
#include "Recast.h" // Only here!
#include "imgui.h"

#include <memory>

NavMeshConfig::NavMeshConfig()
{
}

NavMeshConfig::~NavMeshConfig()
{
}

void NavMeshConfig::ApplyTo(void* out) const
{
    rcConfig& outCfg = *reinterpret_cast<rcConfig*>(out);
    memset(&outCfg, 0, sizeof(rcConfig));

    outCfg.cs                     = settings.cellSize;
    outCfg.ch                     = settings.cellHeight;
    outCfg.walkableSlopeAngle     = settings.walkableSlopeAngle;
    outCfg.walkableClimb          = settings.walkableClimb;
    outCfg.walkableHeight         = settings.walkableHeight;
    outCfg.walkableRadius         = settings.walkableRadius;
    outCfg.maxEdgeLen             = settings.maxEdgeLen;
    outCfg.maxSimplificationError = settings.maxSimplificationError;
    outCfg.minRegionArea          = settings.minRegionArea;
    outCfg.mergeRegionArea        = settings.mergeRegionArea;
    outCfg.maxVertsPerPoly        = settings.maxVertsPerPoly;
    outCfg.detailSampleDist       = settings.detailSampleDist;
    outCfg.detailSampleMaxError   = settings.detailSampleMaxError;
}

bool NavMeshConfig::LoadFromMeta(UID navmeshUID)
{
    const std::string navmeshName     = App->GetLibraryModule()->GetResourceName(navmeshUID);
    const std::string navmeshMetaPath = App->GetProjectModule()->GetLoadedProjectPath() + METADATA_PATH +
                                        NAVMESH_META_PREFIX + navmeshName + META_EXTENSION;
    rapidjson::Document doc;
    bool loaded = FileSystem::LoadJSON(navmeshMetaPath.c_str(), doc);

    if (!doc.HasMember("NavMeshConfig")) return false;

    const auto& cfg                 = doc["NavMeshConfig"];

    settings.cellSize               = cfg["cellSize"].GetFloat();
    settings.cellHeight             = cfg["cellHeight"].GetFloat();
    settings.walkableSlopeAngle     = cfg["walkableSlopeAngle"].GetFloat();
    settings.walkableClimb          = cfg["walkableClimb"].GetInt();
    settings.walkableHeight         = cfg["walkableHeight"].GetInt();
    settings.walkableRadius         = cfg["walkableRadius"].GetInt();
    settings.maxEdgeLen             = cfg["maxEdgeLen"].GetInt();
    settings.maxSimplificationError = cfg["maxSimplificationError"].GetFloat();
    settings.minRegionArea          = cfg["minRegionArea"].GetInt();
    settings.mergeRegionArea        = cfg["mergeRegionArea"].GetInt();
    settings.maxVertsPerPoly        = cfg["maxVertsPerPoly"].GetInt();
    settings.detailSampleDist       = cfg["detailSampleDist"].GetFloat();
    settings.detailSampleMaxError   = cfg["detailSampleMaxError"].GetFloat();

    return true;
}

void NavMeshConfig::RenderEditorUI()
{
    ImGui::Text("Recast Config");

    ImGui::SliderFloat("Cell Size", &settings.cellSize, 0.05f, 1.0f);
    ImGui::SliderFloat("Cell Height", &settings.cellHeight, 0.05f, 1.0f);

    ImGui::SliderFloat("Slope Angle", &settings.walkableSlopeAngle, 0.0f, 90.0f);
    ImGui::SliderInt("Walkable Climb", &settings.walkableClimb, 0, 10);
    ImGui::SliderInt("Walkable Height", &settings.walkableHeight, 0, 10);
    ImGui::SliderInt("Walkable Radius", &settings.walkableRadius, 0, 10);

    ImGui::SliderInt("Max Edge Length", &settings.maxEdgeLen, 0, 50);
    ImGui::SliderFloat("Max Simplification Error", &settings.maxSimplificationError, 0.0f, 5.0f);
    ImGui::SliderInt("Min Region Area", &settings.minRegionArea, 0, 100);
    ImGui::SliderInt("Merge Region Area", &settings.mergeRegionArea, 0, 100);
    ImGui::SliderInt("Max Verts Per Poly", &settings.maxVertsPerPoly, 3, 12);

    const char* partitionLabels[] = {
        "SAMPLE_PARTITION_WATERSHED", "SAMPLE_PARTITION_MONOTONE", "SAMPLE_PARTITION_LAYERS"
    };

    int currentIndex = static_cast<int>(settings.partitionType);
    if (ImGui::Combo("Partition Type", &currentIndex, partitionLabels, IM_ARRAYSIZE(partitionLabels)))
    {
        settings.partitionType = static_cast<SamplePartitionType>(currentIndex);
    }

    ImGui::SliderFloat("Detail Sample Distance", &settings.detailSampleDist, 0.0f, 10.0f);
    ImGui::SliderFloat("Detail Sample Max Error", &settings.detailSampleMaxError, 0.0f, 10.0f);

    ImGui::Separator();
    ImGui::Text("Additional Filters");

    ImGui::Checkbox("Filter Low Hanging Obstacles", &settings.filterLowHangingObstacles);
    ImGui::Checkbox("Filter Ledge Spans", &settings.filterLedgeSpans);
    ImGui::Checkbox("Filter Walkable Low Height Spans", &settings.filterWalkableLowHeightSpans);

    ImGui::Separator();
    ImGui::Text("Agent Parameters");

    ImGui::SliderFloat("Agent Height", &settings.agentHeight, 0.0f, 5.0f);
    ImGui::SliderFloat("Agent Radius", &settings.agentRadius, 0.0f, 5.0f);
    ImGui::SliderFloat("Agent Max Climb", &settings.agentMaxClimb, 0.0f, 5.0f);
}