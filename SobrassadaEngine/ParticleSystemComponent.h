#pragma once

#include "Component.h"

#include <utility>
#include <vector>

class ResourceMaterial;
class ResourceTexture;
class ParticleEmitter;

class ParticleSystemComponent : public Component
{
  public:
    ParticleSystemComponent(UID uid, GameObject* parent);
    ParticleSystemComponent(const rapidjson::Value& initialState, GameObject* parent);
    ~ParticleSystemComponent() override;

    void Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const override;
    void Clone(const Component* other) override;

    void Update(float deltaTime) override;
    void Render(float deltaTime) override;
    void RenderDebug(float deltaTime) override;
    void RenderEditorInspector() override;

    bool IsUsingTexture() const { return useTexture; }
    UID GetMaterialUID() const { return currentMaterialUID; }
    UID GetTextureUID() const { return currentTextureUID; }

  private:
    ParticleEmitter* currentEmitter = nullptr;
    std::vector<std::pair<std::string, ParticleEmitter*>> emitters;

    char newTagName[64]               = "";

    bool useTexture                   = false;
    std::string currentResourceName   = "No material";

    UID currentMaterialUID            = DEFAULT_MATERIAL_UID;
    ResourceMaterial* currentMaterial = nullptr;

    UID currentTextureUID             = FALLBACK_TEXTURE_UID;
    ResourceTexture* currentTexture   = nullptr;
};
