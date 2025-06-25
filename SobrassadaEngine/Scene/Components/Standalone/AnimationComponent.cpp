#include "AnimationComponent.h"

#include "AnimController.h"
#include "Application.h"
#include "CameraModule.h"
#include "EditorUIModule.h"
#include "FileSystem.h"
#include "GameObject.h"
#include "HashString.h"
#include "LibraryModule.h"
#include "ProjectModule.h"
#include "Resource.h"
#include "ResourceAnimation.h"
#include "ResourceModel.h"
#include "ResourcesModule.h"
#include "SceneModule.h"
#include "StateMachineEditor.h"
#include "Animation/AnimationTrigger.h"
#include "AudioModule.h"


#include "Math/Quat.h"
#include "imgui.h"
#include <set>

#ifdef OPTICK
#include "optick.h"
#endif

AnimationComponent::AnimationComponent(const UID uid, GameObject* parent)
    : Component(uid, parent, "Animation", COMPONENT_ANIMATION)
{
    animController = new AnimController();
}

AnimationComponent::AnimationComponent(const rapidjson::Value& initialState, GameObject* parent)
    : Component(initialState, parent)
{
    animController = new AnimController();
    if (initialState.HasMember("Animations") && initialState["Animations"].IsUint64())
    {
        resource            = initialState["Animations"].GetUint64();
        currentAnimResource = static_cast<ResourceAnimation*>(App->GetResourcesModule()->RequestResource(resource));
    }
    else
    {
        resource = INVALID_UID;
    }

    if (initialState.HasMember("StateMachine") && initialState["StateMachine"].IsUint64())
    {
        UID smUID            = initialState["StateMachine"].GetUint64();
        resourceStateMachine = static_cast<ResourceStateMachine*>(App->GetResourcesModule()->RequestResource(smUID));
        if (resourceStateMachine) currentState = resourceStateMachine->GetDefaultState();
    }
    else
    {
        resourceStateMachine = nullptr;
    }

    if (initialState.HasMember("ClipTriggers") && initialState["ClipTriggers"].IsArray())
    {
        for (const auto& obj : initialState["ClipTriggers"].GetArray())
        {
            UID uid        = obj["ClipUID"].GetUint64();
            float key      = obj["KeyTime"].GetFloat();
            TriggerType tp = static_cast<TriggerType>(obj["Type"].GetInt());
            std::string pl = obj["EventName"].GetString();

            clipTriggers[uid].push_back(new AnimationTrigger(key, tp, pl));
        }
    }
}

AnimationComponent::~AnimationComponent()
{
    ReleaseAllTriggers();
    delete animController;
    App->GetResourcesModule()->ReleaseResource(currentAnimResource);
}

void AnimationComponent::Init()
{
    currentAnimResource = static_cast<ResourceAnimation*>(App->GetResourcesModule()->RequestResource(resource));
    currentAnimName     = App->GetLibraryModule()->GetResourceName(resource);
}

void AnimationComponent::OnPlay(bool isTransition)
{
    playing                 = true;
    unsigned transitionTime = 0;
    if (animController != nullptr)
    {
        if (resourceStateMachine)
        {
            for (const auto& state : resourceStateMachine->states)
            {
                if (state.name == currentState->name)
                {
                    for (const auto& transition : resourceStateMachine->transitions)
                    {
                        if (state.name == transition.toState)
                        {
                            transitionTime = transition.interpolationTime;
                        }
                    }
                    for (const auto& clip : resourceStateMachine->clips)
                    {
                        if (clip.clipName == currentState->clipName)
                        {
                            if (isTransition && transitionTime > 0)
                                animController->SetTargetAnimationResource(
                                    clip.animationResourceUID, transitionTime, clip.loop, clip.animationSpeed
                                );
                            else animController->Play(clip.animationResourceUID, clip.loop, clip.animationSpeed);
                            resource = clip.animationResourceUID;
                        }

                        
                    }
                }
            }
        }
        else animController->Play(resource, true, defaultTime);

        lastTime = 0.0f;
        for (AnimationTrigger* trgg : clipTriggers[resource])
            trgg->Reset();
    }
}

void AnimationComponent::OnStop()
{
    playing     = false;
    currentTime = 0.0f;
    if (animController != nullptr)
    {
        animController->Stop();
        if (resourceStateMachine) currentState = resourceStateMachine->GetDefaultState();
    }
}

void AnimationComponent::OnPause()
{
    playing = false;
    if (animController != nullptr)
    {
        animController->Pause();
    }
}

void AnimationComponent::OnResume()
{
    playing = true;
    if (animController != nullptr)
    {
        animController->Resume();
    }
}

void AnimationComponent::Render(float deltaTime)
{
    if (!IsEffectivelyEnabled()) return;
}

void AnimationComponent::RenderDebug(float deltaTime)
{
}

void AnimationComponent::RenderEditorInspector()
{
    Component::RenderEditorInspector();

    std::string originAnimation = "";
    if (resource != 0)
    {
        const size_t underscorePos = currentAnimName.find('_');
        if (underscorePos != std::string::npos) originAnimation = currentAnimName.substr(0, underscorePos);
        if (currentAnimResource != nullptr)
        {
            ImGui::Text("Animation: %s", currentAnimName.c_str());
            ImGui::Text("Duration: %.2f seconds", currentAnimResource->GetDuration());

            if (animController != nullptr && ImGui::TreeNode("Channels"))
            {
                for (const auto& channel : currentAnimResource->channels)
                {
                    ImGui::Text("Bone: %s", channel.first.c_str());

                    if (ImGui::TreeNode(channel.first.c_str()))
                    {
                        const Channel& ch = channel.second;

                        ImGui::Text("Positions: %d", ch.numPositions);
                        if (ch.numPositions > 0)
                        {
                            ImGui::Text("First Position Time: %.2f", ch.posTimeStamps.front());
                            ImGui::Text("Last Position Time: %.2f", ch.posTimeStamps.back());
                        }

                        ImGui::Text("Rotations: %d", ch.numRotations);
                        if (ch.numRotations > 0)
                        {
                            ImGui::Text("First Rotation Time: %.2f", ch.rotTimeStamps.front());
                            ImGui::Text("Last Rotation Time: %.2f", ch.rotTimeStamps.back());
                        }

                        ImGui::TreePop();
                    }
                }
                ImGui::TreePop();
            }
        }
        else
        {
            ImGui::Text("Animation resource not found");
        }
    }
    else
    {
        ImGui::Text("No animation assigned");
    }

    if (ImGui::CollapsingHeader("Object Selection", ImGuiTreeNodeFlags_DefaultOpen))
    {

        ImGui::Text("Selected Object: %s", parent->GetName().c_str());

        if (currentAnimResource)
        {
            ImGui::Text("Current Animation: %s", currentAnimResource->GetName().c_str());
            animationDuration = currentAnimResource->GetDuration();

            // Display animation controls
            ImGui::Separator();
            ImGui::Text("Animation Controls");

            if (ImGui::Button("Play")) OnPlay(false);

            ImGui::SameLine();

            if (ImGui::Button("Pause")) OnPause();

            ImGui::SameLine();

            if (ImGui::Button("Stop")) OnStop();

            if (ImGui::Button("Resume")) OnResume();

            if (ImGui::SliderFloat("Timeline", &currentTime, 0.0f, animationDuration, "%.2f sec"))
            {
                // When user manually changes the time, update the animation controller
                if (GetAnimationController())
                {
                    GetAnimationController()->SetTime(currentTime);
                }
            }

            // Bone visualization
            if (ImGui::TreeNode("Bone Mapping"))
            {

                const auto& boneMap = GetBoneMapping();

                if (boneMap.empty())
                {
                    ImGui::Text("No bones mapped. Animation might not apply correctly.");
                }
                else
                {
                    ImGui::Text("%d bones mapped:", boneMap.size());
                    for (const auto& pair : boneMap)
                    {
                        bool foundInAnimation = false;

                        if (GetCurrentAnimation())
                        {
                            foundInAnimation = GetCurrentAnimation()->channels.find(pair.first) !=
                                               GetCurrentAnimation()->channels.end();
                        }

                        ImGui::TextColored(
                            foundInAnimation ? ImVec4(0, 1, 0, 1) : ImVec4(1, 0, 0, 1), "%s -> %s", pair.first.c_str(),
                            pair.second ? pair.second->GetName().c_str() : "NULL"
                        );
                    }
                }

                ImGui::TreePop();
            }
        }
    }

    if (ImGui::CollapsingHeader("Animation Library"))
    {

        ImGui::Text("Available Animations:");
        const std::unordered_map<HashString, UID>& animationMap = App->GetLibraryModule()->GetAnimMap();

        for (const auto& pair : animationMap)
        {
            const std::string& animationName = pair.first.GetString();

            if (animationName.rfind(originAnimation, 0) == 0)
            {
                const bool isSelected = (currentAnimName == animationName);

                if (ImGui::Selectable(animationName.c_str(), isSelected))
                {
                    currentAnimName = pair.first.GetString();
                    resource        = pair.second;

                    if (playing)
                    {
                        playing     = false;
                        currentTime = 0.0f;
                        OnStop();
                    }
                }
            }
        }
    }

    if (ImGui::CollapsingHeader("Animation Triggers", ImGuiTreeNodeFlags_DefaultOpen))
    {
        UID clipUID                        = resource;
        std::vector<AnimationTrigger*>& vec = clipTriggers[resource]; 
        for (size_t i = 0; i < vec.size(); ++i)
        {
            AnimationTrigger* trgg = vec[i];
            ImGui::PushID(static_cast<int>(i));

            //Seconds inside clip
            float t = trgg->GetTime();
            if (ImGui::SliderFloat("Time", &t, 0.f, currentAnimResource->GetDuration(), "%.2f")) 
                trgg->SetTime(t);

            const char* types[] = {"Sound" /* Rest of different triggers */};
            int currentType     = static_cast<int>(trgg->GetType());
            if (ImGui::Combo("Type", &currentType, types, IM_ARRAYSIZE(types)))
                trgg->SetType(static_cast<TriggerType>(currentType));

            //char buf[128];
            //strncpy_s(buf, trgg->GetData().c_str(), sizeof(buf));
            //if (ImGui::InputText("TriggerName", buf, IM_ARRAYSIZE(buf)))
            //    *trgg = AnimationTrigger(t, TriggerType::SOUND, buf);


            if (ImGui::Button("Delete"))
            {
                RemoveTrigger(clipUID, i);
                ImGui::PopID();
                break;
            }

            ImGui::Separator();
            ImGui::PopID();
        }

        if (vec.empty())
            ImGui::TextDisabled("No triggers yet.");
        
        if (ImGui::Button("Add Trigger"))
            AddSoundTrigger(clipUID, 0.0f, "");
    }

    ImGui::Separator();
    ImGui::Text("Associated State Machine");

    const std::unordered_map<HashString, UID>& stateMap = App->GetLibraryModule()->GetStateMachineMap();

    std::string currentName                             = "None";
    if (resourceStateMachine) currentName = resourceStateMachine->GetName();

    if (ImGui::BeginCombo("##StateMachineCombo", currentName.c_str()))
    {
        for (const auto& [name, uid] : stateMap)
        {
            const bool isSelected = (resourceStateMachine && resourceStateMachine->GetUID() == uid);
            if (ImGui::Selectable(name.c_str(), isSelected))
            {
                if (resourceStateMachine) App->GetResourcesModule()->ReleaseResource(resourceStateMachine);
                resourceStateMachine =
                    static_cast<ResourceStateMachine*>(App->GetResourcesModule()->RequestResource(uid));
                if (resourceStateMachine) currentState = resourceStateMachine->GetDefaultState();
            }
            if (isSelected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    if (resourceStateMachine)
    {
        ImGui::Separator();
        ImGui::Text("Available Triggers:");

        for (const std::string& triggerName : resourceStateMachine->availableTriggers)
        {
            if (ImGui::Button(triggerName.c_str()))
            {
                // GLOG("Trigger selected: %s", triggerName.c_str());
                bool triggerAvailable = false;
                if (IsPlaying())
                {
                    // triggerAvailable = resourceStateMachine->UseTrigger(triggerName);
                    triggerAvailable = resourceStateMachine->UseTrigger(triggerName, currentState);
                    if (triggerAvailable)
                    {
                        OnPlay(true);
                    }
                }
            }
        }
    }

    if (playing && GetAnimationController())
    {
        currentTime = GetAnimationController()->GetTime();

        if (currentTime >= animationDuration)
        {
            currentTime = 0.0f;
        }
    }
}

void AnimationComponent::DrawTriggerInspector()
{
    UID clipUID                         = resource;
    std::vector<AnimationTrigger*>& vec = clipTriggers[resource];

    const auto& eventNames              = App->GetAudioModule()->GetEventNames();
    if (ImGui::Button("Add Trigger"))
    {
        const std::string& defName = eventNames.empty() ? std::string() : eventNames.front();

        AddSoundTrigger(clipUID, 0.f, defName); //Sound by default
    }

    if (vec.empty())
    {
        ImGui::TextDisabled("No triggers yet.");
        return;
    }

    for (size_t i = 0; i < vec.size(); ++i)
    {
        AnimationTrigger* trgg = vec[i];
        ImGui::PushID(static_cast<int>(i));

        // Seconds inside clip
        float t = trgg->GetTime();
        if (ImGui::SliderFloat("Time", &t, 0.f, currentAnimResource->GetDuration(), "%.2f")) trgg->SetTime(t);

        const char* types[] = {"Sound" /* Rest of different triggers */};
        int currentType     = static_cast<int>(trgg->GetType());
        if (ImGui::Combo("Type", &currentType, types, IM_ARRAYSIZE(types)))
            trgg->SetType(static_cast<TriggerType>(currentType));

        switch (trgg->GetType())
        {
        case TriggerType::SOUND:
        {
            static std::vector<const char*> cstrs;
            cstrs.clear();
            for (const std::string& s : eventNames)
                cstrs.push_back(s.c_str());

            int sel = 0;
            for (size_t n = 0; n < eventNames.size(); ++n)
                if (eventNames[n] == trgg->GetName())
                {
                    sel = static_cast<int>(n);
                    break;
                }

            if (ImGui::Combo("Sound Event", &sel, cstrs.data(), static_cast<int>(cstrs.size())))
            {
                trgg->SetName(eventNames[sel]);
            }
            break;
        }
            // rest of events
        }

        if (ImGui::Button("Delete"))
        {
            RemoveTrigger(clipUID, i);
            ImGui::PopID();
            break;
        }

        ImGui::Separator();
        ImGui::PopID();
    }

    


    
}

void AnimationComponent::Clone(const Component* other)
{
    if (other->GetType() == ComponentType::COMPONENT_ANIMATION)
    {
        const AnimationComponent* otherAnimation = static_cast<const AnimationComponent*>(other);
        enabled                                  = otherAnimation->enabled;
        wasEnabled                               = otherAnimation->wasEnabled;

        resource                                 = otherAnimation->resource;
        AddAnimation(resource);

        if (otherAnimation->resourceStateMachine)
        {
            resourceStateMachine = otherAnimation->resourceStateMachine;
            resourceStateMachine->AddReference();
            currentState = resourceStateMachine->GetDefaultState();
        }

        clipTriggers.clear(); 

        for (const auto& [uid, srcList] : otherAnimation->clipTriggers)
        {
            auto& dstList = clipTriggers[uid];
            dstList.reserve(srcList.size());

            for (const AnimationTrigger* srcTrig : srcList)
            {
                AnimationTrigger* newTrig = new AnimationTrigger(*srcTrig);

                newTrig->Reset();
                dstList.push_back(newTrig);
            }
        }
    }
    else
    {
        GLOG("It is not possible to clone a component of a different type!");
    }
}

void AnimationComponent::Update(float deltaTime)
{
#ifdef OPTICK
    OPTICK_CATEGORY("Application::AnimationUpdate", Optick::Category::GameLogic)
#endif // OPTICK
    {
        if (!IsEffectivelyEnabled()) return;
        if (!animController->IsPlaying()) return;

        if (boneMapping.empty())
        {
            SetBoneMapping();
        }

        animController->Update(deltaTime);

        CheckTriggers();

        std::set<GameObject*> modifiedBones;

        for (auto& channel : currentAnimResource->channelNames)
        {
            const HashString& boneName = channel;

            auto boneIt = boneMapping.find(boneName);

            if (boneIt != boneMapping.end())
            {
                GameObject* bone          = boneIt->second;

                // Get current transform components
                // if the animation doesn't provide values
                float4x4 currentTransform = bone->GetLocalTransform();
                float3 position           = currentTransform.TranslatePart();
                Quat rotation             = Quat(currentTransform.RotatePart());
                float3 scale              = currentTransform.GetScale();

                // Pass CURRENT values to GetTransform - it will only modify them
                // if the animation has data for that channel type

                animController->GetTransform(boneName, position, rotation);
                rotation.Normalize();

                float4x4 transformMatrix = float4x4::FromTRS(position, rotation, scale);

                bone->SetJustLocalTransform(transformMatrix);
                modifiedBones.insert(bone);
            }
        }

        parent->UpdateTransformForGOBranch();
    }
}

void AnimationComponent::Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const
{
    Component::Save(targetState, allocator);

    targetState.AddMember(
        "Animations", currentAnimResource != nullptr ? currentAnimResource->GetUID() : INVALID_UID, allocator
    );
    targetState.AddMember(
        "StateMachine", resourceStateMachine != nullptr ? resourceStateMachine->GetUID() : INVALID_UID, allocator
    );

    rapidjson::Value trigArr(rapidjson::kArrayType);

    for (const auto& [uid, list] : clipTriggers)
    {
        for (const AnimationTrigger* trgg : list)
        {
            rapidjson::Value obj(rapidjson::kObjectType);
            obj.AddMember("ClipUID", uid, allocator);
            obj.AddMember("KeyTime", trgg->GetTime(), allocator);
            obj.AddMember("Type", static_cast<int>(trgg->GetType()), allocator);
            obj.AddMember("EventName", rapidjson::Value(trgg->GetName().c_str(), allocator), allocator);
            trigArr.PushBack(obj, allocator);
        }
    }

    targetState.AddMember("ClipTriggers", trigArr, allocator);
}

void AnimationComponent::AddAnimation(UID animationUID)
{
    if (animationUID == INVALID_UID) return;
    if (currentAnimResource != nullptr && currentAnimResource->GetUID() == animationUID) return;

    ResourceAnimation* newAnimation =
        dynamic_cast<ResourceAnimation*>(App->GetResourcesModule()->RequestResource(animationUID));

    if (newAnimation != nullptr)
    {
        App->GetResourcesModule()->ReleaseResource(currentAnimResource);
        currentAnimResource = newAnimation;
        currentAnimName     = currentAnimResource->GetName();
        resource            = animationUID;
        SetBoneMapping();
    }
}

bool AnimationComponent::IsPlaying() const
{
    return animController ? animController->IsPlaying() : false;
}

void AnimationComponent::SetAnimationResource(UID animResource)
{
    resource = animResource;
    AddAnimation(resource);
    // GLOG("Setting animation resource: %llu", resource);
}

void AnimationComponent::UpdateBoneHierarchy(GameObject* bone)
{
    if (!bone) return;

    // global transform is updated
    bone->OnTransformUpdated();

    // Debug output to see what's happening
    // GLOG("Updated bone %s global transform", bone->GetName().c_str());

    for (const UID childUID : bone->GetChildren())
    {
        GameObject* child = App->GetSceneModule()->GetScene()->GetGameObjectByUID(childUID);
        if (child)
        {
            UpdateBoneHierarchy(child);
        }
    }
}

void AnimationComponent::SetBoneMapping()
{
    boneMapping.clear();
    bindPoseTransforms.clear();

    std::function<void(GameObject*)> mapBones = [this, &mapBones](GameObject* obj)
    {
        if (obj == nullptr) return;

        boneMapping[obj->GetName()]        = obj;
        bindPoseTransforms[obj->GetName()] = obj->GetLocalTransform(); // Store bind pose

        for (const UID childUID : obj->GetChildren())
        {
            GameObject* child = App->GetSceneModule()->GetScene()->GetGameObjectByUID(childUID);
            if (child != nullptr)
            {
                mapBones(child);
            }
        }
    };
    mapBones(parent);

    // GLOG("Bone mapping completed: %zu bones mapped", boneMapping.size());
}

bool AnimationComponent::IsFinished() const
{
    return animController->IsFinished();
}

bool AnimationComponent::UseTrigger(const std::string& triggerName)
{
    bool triggerDone = false;
    if (resourceStateMachine)
    {
        // triggerDone = resourceStateMachine->UseTrigger(triggerName);
        triggerDone = resourceStateMachine->UseTrigger(triggerName, currentState);
        if (triggerDone)
        {
            OnPlay(true);
        }
    }
    return triggerDone;
}

void AnimationComponent::AddSoundTrigger(UID clipUID, float atSeconds, const std::string& eventName)
{
    clipTriggers[clipUID].push_back(new AnimationTrigger(atSeconds, TriggerType::SOUND, eventName));
}

void AnimationComponent::RemoveTrigger(UID clipUID, size_t index)
{
    auto it = clipTriggers.find(clipUID);
    if (it == clipTriggers.end()) return;

    auto& vec = it->second;
    if (index >= vec.size()) return;

    delete vec[index];
    vec.erase(vec.begin() + index);
}

void AnimationComponent::ClearTriggers(UID clipUID)
{
    ReleaseClipTriggers(clipUID);
}

void AnimationComponent::CheckTriggers()
{
    if (!currentAnimResource) return;

    UID clipUID = currentAnimResource->GetUID();
    std::vector<AnimationTrigger*>& vec   = clipTriggers[clipUID];
    float now   = animController->GetTime();
    bool looped = now < lastTime;

    for (AnimationTrigger* trgg : vec)
    {
        if (trgg->Check(lastTime, now, looped))
        {
            if (trgg->GetType() == TriggerType::SOUND) 
                App->GetAudioModule()->EmitEvent(trgg->GetName(), GetParentUID());
        }
    }

    lastTime = now;
}

void AnimationComponent::ReleaseClipTriggers(UID clipUID)
{
    auto it = clipTriggers.find(clipUID);
    if (it == clipTriggers.end()) return;

    for (AnimationTrigger* t : it->second)
        delete t;
    it->second.clear();
}

void AnimationComponent::ReleaseAllTriggers()
{
    for (auto& [uid, vec] : clipTriggers)
        for (AnimationTrigger* t : vec)
            delete t;
    clipTriggers.clear();
}
