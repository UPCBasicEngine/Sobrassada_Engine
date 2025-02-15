#pragma once

#include "../LightComponent.h"

#include <Libs/rapidjson/document.h>

class PointLight : public LightComponent
{
  public:
    PointLight(UID uid, UID uidParent, UID uidRoot, const Transform &parentGlobalTransform);
    PointLight(const rapidjson::Value& initialState);
    ~PointLight();

    virtual void Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const;


    void RenderEditorInspector() override;
    void Render() override;

    float GetRange() const { return range; }

  private:
    float range;
    int gizmosMode;
};
