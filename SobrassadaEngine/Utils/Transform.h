#pragma once
#include "imgui.h"

#include <Math/float3.h>

struct Transform    // TODO Change it to a class if it is able to render its own editor view
{
    Transform(): position(0, 0, 0), rotation(0, 0, 0), scale(1, 1, 1) {}

public:

    void RenderEditor()
    {
        ImGui::InputFloat3( "Position", &position[0] );
        ImGui::InputFloat3( "Rotation", &rotation[0] ); // TODO Add option to switch between degrees and radians
        ImGui::InputFloat3( "Scale", &scale[0] );
    }

public:
    
    float3 position;
    float3 rotation;
    float3 scale;
};
