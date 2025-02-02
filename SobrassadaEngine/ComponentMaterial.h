#pragma once
#include "imgui.h"
#include "Material.h"
class ComponentMaterial
{
  public:
    void OnEditorUpdate();

  private:
    Material material;
 
};
