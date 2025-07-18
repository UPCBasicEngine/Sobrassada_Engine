#pragma once
#include "EngineEditorBase.h"
#include "Geometry/OBB.h"
#include "Globals.h"
#include "imgui.h"
#include <string>
#include <vector>


class LightprobeEditor : public EngineEditorBase
{
  public:
    LightprobeEditor(const std::string& editorName, UID uid)
        : EngineEditorBase(editorName, uid)
         
    {
    }

    ~LightprobeEditor() override;

  private:
    bool RenderEditor() override;

  private:
    UID uid;
};
