#include "LightprobeEditor.h"

LightprobeEditor::~LightprobeEditor()
{
}

bool LightprobeEditor::RenderEditor()
{
    if (!EngineEditorBase::RenderEditor()) return false;

    ImGui::Begin(name.c_str());
    ImGui::MenuItem("Add lightprobe");
}
