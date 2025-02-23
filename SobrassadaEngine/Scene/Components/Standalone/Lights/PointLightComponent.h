#pragma once

#include "../LightComponent.h"

#include <Libs/rapidjson/document.h>

class PointLightComponent : public LightComponent
{
  public:
    PointLightComponent(UID uid, UID uidParent, UID uidRoot, const Transform &parentGlobalTransform);
    PointLightComponent(const rapidjson::Value& initialState);
    ~PointLightComponent();

    virtual void Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const;


    void RenderEditorInspector() override;
    void Render() override;

    float GetRange() const { return range; }

  private:
    float range;
    int gizmosMode;
};
