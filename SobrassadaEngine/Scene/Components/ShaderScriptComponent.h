#pragma once

#include "Component.h"

class Script;
class GameObject;

class ShaderScriptComponent : public Component
{
  public:
    ShaderScriptComponent(UID uid, GameObject* parent);
    ShaderScriptComponent(const rapidjson::Value& initialState, GameObject* parent);
    ~ShaderScriptComponent();

    void Load(const rapidjson::Value& initialState);

    void Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const override;
    void Clone(const Component* other) override;

    void Update(float deltaTime) override;
    void Render(float deltaTime, CameraComponent* camera) override;
    void RenderDebug(float deltaTime) override;
    void RenderEditorInspector() override;

    void InitScriptInstances();

    bool CreateScript(const std::string& scriptType);
    void DeleteScript(const int index);
    void DeleteAllScripts();

    void ResetInitializationFlags();

    const std::vector<Script*>& GetScriptInstances() const { return scriptInstances; }
    const std::vector<std::string>& GetAllScriptNames() const { return scriptNames; }

    template <typename T> T* GetScriptByType()
    {
        for (Script* script : scriptInstances)
        {
            T* currentScript = dynamic_cast<T*>(script);
            if (currentScript) return currentScript;
        }

        return nullptr;
    }

    void SetComponentEnabled(bool value);

  private:
    bool startScript = false;

    std::vector<std::string> scriptNames;
    std::vector<Script*> scriptInstances;
    std::vector<bool> scriptEnabled;
    std::vector<bool> scriptInitialized;
    std::vector<bool> scriptWasEnabledLastFrame;
    std::vector<int> scriptTypes;
};
