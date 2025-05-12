#pragma once

#include "Component.h"
#include "Globals.h"

#include "rapidjson/document.h"
#include <map>
#include <unordered_map>

class ResourceAnimation;
class ResourceStateMachine;
class AnimController;
class GameObject;

struct State;

class SOBRASADA_API_ENGINE AnimationComponent : public Component
{
  public:
    AnimationComponent(UID uid, GameObject* parent);
    AnimationComponent(const rapidjson::Value& initialState, GameObject* parent);
    ~AnimationComponent() override;

    void Init() override;
    void Clone(const Component* other) override;
    void Update(float deltaTime) override;
    void Render(float deltaTime) override;
    void RenderDebug(float deltaTime) override;
    void RenderEditorInspector() override;
    void Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const override;

    void OnPlay(bool isTransition);
    void OnStop();
    void OnPause();
    void OnResume();
    void AddAnimation(UID resource);
    bool UseTrigger(const std::string& triggerName);

    UID GetAnimationResource() const { return resource; }
    ResourceAnimation* GetCurrentAnimation() const { return currentAnimResource; }
    AnimController* GetAnimationController() { return animController; }
    ResourceStateMachine* GetResourceStateMachine() const { return resourceStateMachine; }
    const std::unordered_map<std::string, GameObject*>& GetBoneMapping() const { return boneMapping; }
    bool IsPlaying() const;
    bool IsFinished() const;

    void SetAnimationResource(UID animResource);
    void UpdateBoneHierarchy(GameObject* bone);
    void SetBoneMapping();

  private:
    UID resource                               = INVALID_UID;
    std::string currentAnimName                = "None";

    AnimController* animController             = nullptr;
    ResourceAnimation* currentAnimResource     = nullptr;
    ResourceStateMachine* resourceStateMachine = nullptr;
    const State* currentState                  = nullptr;

    std::unordered_map<std::string, GameObject*> boneMapping;
    std::map<std::string, float4x4> bindPoseTransforms;

    float animationDuration = 0.0f;
    bool playing            = false;
    float currentTime       = 0.0f;
    float fadeTime          = 0.0f;
};