#pragma once

#include "Component.h"
#include "Globals.h"
#include "ResourceStateMachine.h"

#include "rapidjson/document.h"
#include <map>
#include <unordered_map>

class ResourceAnimation;
class AnimController;
class GameObject;
class AnimationTrigger;

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
    void DrawTriggerInspector();

    void OnPlay(bool isTransition);
    void OnStop();
    void OnPause();
    void OnResume();
    void AddAnimation(UID resource);
    bool UseTrigger(const std::string& triggerName);
    void AddSoundTrigger(UID clipUID, float atSeconds, const std::string& eventName, bool repeat = true);
    void RemoveTrigger(UID clipUID, size_t index);
    void ClearTriggers(UID clipUID);

    UID GetAnimationResource() const { return resource; }
    const HashString& GetCurrentStateName() const { return currentState->name; }
    ResourceAnimation* GetCurrentAnimation() const { return currentAnimResource; }
    AnimController* GetAnimationController() { return animController; }
    ResourceStateMachine* GetResourceStateMachine() const { return resourceStateMachine; }
    const std::unordered_map<HashString, GameObject*>& GetBoneMapping() const { return boneMapping; }
    bool IsPlaying() const;
    bool IsFinished() const;

    void SetAnimationResource(UID animResource);
    void UpdateBoneHierarchy(GameObject* bone);
    void SetBoneMapping();

  private:
    void CheckTriggers(float prevTime, float currTime);
    void ReleaseClipTriggers(UID clipUID);
    void ReleaseAllTriggers();

  private:
    UID resource                               = INVALID_UID;
    std::string currentAnimName                = "None";

    AnimController* animController             = nullptr;
    ResourceAnimation* currentAnimResource     = nullptr;
    ResourceStateMachine* resourceStateMachine = nullptr;
    const State* currentState                  = nullptr;

    std::unordered_map<HashString, GameObject*> boneMapping;
    std::map<std::string, float4x4> bindPoseTransforms;
    std::unordered_map<UID, std::vector<AnimationTrigger*>> clipTriggers;

    float lastTime          = 0.0f;
    float animationDuration = 0.0f;
    bool playing            = false;
    float currentTime       = 0.0f;
    float defaultTime       = 1.0f;
    float fadeTime          = 0.0f;
    float triggerMuteTime   = 0.0f;
};