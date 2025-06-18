#include "StateMachineEditor.h"

#include "Application.h"
#include "Components/Standalone/AnimationComponent.h"
#include "FileSystem/StateMachineManager.h"
#include "LibraryModule.h"
#include "ResourcesModule.h"
#include "StateNode.h"

StateMachineEditor::StateMachineEditor(const std::string& editorName, UID uid, ResourceStateMachine* stateMachine)
    : EngineEditorBase(editorName, uid), uid(uid), resource(stateMachine)
{
    graph        = std::make_unique<ImFlow::ImNodeFlow>("StateMachineGraph_" + std::to_string(uid));

    ImVec2 pos   = ImVec2(0, 0);

    auto newNode = graph->placeNodeAt<StateNode>(pos);
    if (newNode)
    {
        auto newStateNode = std::dynamic_pointer_cast<StateNode>(newNode);
        if (newStateNode)
        {
            CreateBaseState(*newStateNode.get());
        }
    }
}

StateMachineEditor::~StateMachineEditor()
{
    if (graph)
    {
        for (const auto& [uid, node] : graph->getNodes())
        {
            if (node)
            {
                for (const auto& pin : node->getIns())
                {
                    pin->deleteLink();
                }

                for (const auto& pin : node->getOuts())
                {
                    pin->deleteLink();
                }
            }
        }
        graph->getNodes().clear();
    }
}

bool StateMachineEditor::RenderEditor()
{
    if (!EngineEditorBase::RenderEditor()) return false;

    ImGui::Begin(name.c_str());

    if (ImGui::Button("Save"))
    {
        SaveMachine();
    }
    ShowSavePopup();
    ImGui::SameLine();
    if (ImGui::Button("Load"))
    {
        LoadMachine();
    }
    ShowLoadPopup();
    ImGui::SameLine();
    if (ImGui::Button("Triggers"))
    {
        ShowTriggers();
    }
    ShowTriggersPopup();

    const State* activeState = resource->GetDefaultState();
    for (const auto& pair : graph->getNodes())
    {
        auto stateNode = std::dynamic_pointer_cast<StateNode>(pair.second);
        if (!stateNode) continue;

        if (activeState && stateNode->GetStateName() == activeState->name.GetString())
        {
            stateNode->SetColor(ImColor(0.58f, 0.573f, 0.075f)); // Verde
        }
        else
        {
            stateNode->SetColor(ImColor(0.2f, 0.4f, 0.8f)); // Azul
        }

        const auto& node = pair.second;
        if (node->isSelected())
        {
            selectedNode = dynamic_cast<StateNode*>(node.get());
        }
    }

    graph->update();

    graph->rightClickPopUpContent(
        [this](ImFlow::BaseNode* node)
        {
            if (node != nullptr)
            {
                if (ImGui::MenuItem("Delete State"))
                {
                    StateNode* stateNode = dynamic_cast<StateNode*>(node);
                    if (stateNode)
                    {
                        RemoveStateNode(*stateNode);
                    }
                }
            }
            else if (node == nullptr && ImGui::MenuItem("Add State"))
            {
                ImVec2 pos   = graph->screen2grid(ImGui::GetMousePos());

                auto newNode = graph->placeNodeAt<StateNode>(pos);
                if (newNode)
                {
                    auto newStateNode = std::dynamic_pointer_cast<StateNode>(newNode);
                    if (newStateNode)
                    {
                        CreateBaseState(*newStateNode.get());
                    }
                }
            }
        }
    );

    DetectNewTransitions();

    graph->droppedLinkPopUpContent(
        [this](ImFlow::Pin* dragged)
        {
            if (dragged && ImGui::MenuItem("Create Transition"))
            {
                auto sourceNode = dynamic_cast<StateNode*>(dragged->getParent());
                if (!sourceNode) return;

                auto newNode = graph->placeNode<StateNode>();
                if (newNode)
                {
                    auto inputPinRaw = std::dynamic_pointer_cast<StateNode>(newNode);
                    if (inputPinRaw)
                    {
                        CreateBaseState(*inputPinRaw.get());
                        resource->AddTransition(
                            sourceNode->GetStateName(), inputPinRaw->GetStateName(), "Trigger", 200
                        );
                        // availableTriggers.push_back("Trigger");
                        // auto newLink =
                        //  std::make_shared<ImFlow::Link>(dragged, inputPinRaw->getInputPin().get(), graph.get());
                        // graph->addLink(newLink);
                        dragged->createLink(inputPinRaw->getInputPin().get());
                    }
                }
            }
        }
    );

    ImGui::End();

    // INSPECTOR
    if (selectedNode != nullptr)
    {
        if (ImGui::IsKeyPressed(ImGuiKey_Delete) && !selectedNode->toDestroy())
        {
            DeleteStateResource(*selectedNode);
            selectedNode->destroy();
        }

        if (!selectedNode->toDestroy())
        {
            ShowInspector();
        }
        else
        {
            selectedNode = nullptr;
        }
    }

    return true;
}

void StateMachineEditor::ShowInspector()
{
    bool rechargeTransition = false;
    ImGui::Begin("State Inspector");

    ImGui::Text("State");

    const State* defaultState = resource->GetDefaultState();
    if (defaultState)
    {
        if (selectedNode->GetStateName() == defaultState->name.GetString())
        {
            ImGui::SameLine();
            ImGui::Text("- Default");
        }
    }
    const State* activeState = resource->GetDefaultState();
    if (activeState)
    {
        if (selectedNode->GetStateName() == activeState->name.GetString())
        {
            ImGui::SameLine();
            ImGui::Text("- Active");
        }
    }

    ImGui::Separator();

    const std::string& stateName = selectedNode->GetStateName();
    char nameBuffer[128]         = "";
    strncpy_s(nameBuffer, stateName.c_str(), sizeof(nameBuffer));
    nameBuffer[sizeof(nameBuffer) - 1] = '\0';

    if (ImGui::InputText("State Name", nameBuffer, sizeof(nameBuffer)))
    {
        if (ImGui::IsItemDeactivatedAfterEdit())
        {
            std::string newName(nameBuffer);

            if (newName != stateName)
            {
                const State* existingState = resource->GetState(newName);
                if (existingState != nullptr)
                {
                    ImGui::TextColored(
                        ImVec4(1, 0.5f, 0.5f, 1.0f), "A state with the name \"%s\" already exists!", nameBuffer
                    );
                }
                else
                {
                    auto transitionsCopy = resource->transitions;

                    for (const auto& transition : transitionsCopy)
                    {
                        if (transition.fromState.GetString() == stateName)
                        {
                            const std::string& prevTrigger = transition.triggerName.GetString();
                            uint32_t prevInterpolation     = transition.interpolationTime;
                            resource->RemoveTransition(
                                transition.fromState.GetString(), transition.toState.GetString()
                            );
                            resource->AddTransition(
                                newName, transition.toState.GetString(), prevTrigger, prevInterpolation
                            );
                        }
                        if (transition.toState.GetString() == stateName)
                        {
                            const std::string& prevTrigger = transition.triggerName.GetString();
                            uint32_t prevInterpolation     = transition.interpolationTime;
                            resource->RemoveTransition(
                                transition.toState.GetString(), transition.fromState.GetString()
                            );
                            resource->AddTransition(
                                transition.fromState.GetString(), newName, prevTrigger, prevInterpolation
                            );
                        }
                    }
                    resource->EditState(stateName, newName, selectedNode->GetClipName());
                    selectedNode->SetStateName(newName);
                }
            }
        }
    }
    char clipBuffer[128];
    strncpy_s(clipBuffer, selectedNode->GetClipName().c_str(), sizeof(clipBuffer));
    clipBuffer[sizeof(clipBuffer) - 1] = '\0';

    const auto& animMap                = App->GetLibraryModule()->GetAnimMap();
    std::vector<std::string> animationNames;
    animationNames.reserve(animMap.size());

    for (const auto& [name, uid] : animMap)
    {
        animationNames.push_back(name.GetString());
    }

    int currentClipIndex        = -1;
    std::string currentClipName = selectedNode->GetClipName();

    for (size_t i = 0; i < animationNames.size(); ++i)
    {
        if (animationNames[i] == currentClipName)
        {
            currentClipIndex = static_cast<int>(i);
            break;
        }
    }

    if (ImGui::Combo(
            "Associated Clip", &currentClipIndex,
            [](void* data, int idx, const char** out_text)
            {
                auto* names = static_cast<std::vector<std::string>*>(data);
                if (out_text) *out_text = (*names)[idx].c_str();
                return true;
            },
            &animationNames, (int)animationNames.size()
        ))
    {
        if (currentClipIndex >= 0 && animationNames[currentClipIndex] != currentClipName)
        {
            const std::string& newClipName = animationNames[currentClipIndex];
            UID newClipUID                 = App->GetLibraryModule()->GetAnimUID(newClipName);
            // resource->EditClipInfo(currentClipName, newClipUID, newClipName, false);
            resource->AddClip(newClipUID, newClipName, false);
            resource->EditState(selectedNode->GetStateName(), selectedNode->GetStateName(), newClipName);
            selectedNode->SetClipName(newClipName);

            currentClipName = newClipName;
        }
    }

    ImGui::Spacing();
    ImGui::Text("Clip Information");
    ImGui::Separator();
    const Clip* clip = resource->GetClip(selectedNode->GetClipName());
    if (clip)
    {
        bool loopBuffer = clip->loop;
        float editableSpeed = clip->animationSpeed;
        
        ImGui::Text("Clip UID: %llu", clip->animationResourceUID);
        
        ImGui::Text("Clip Name: %s", clip->clipName.GetString().c_str());
        if(ImGui::SliderFloat("Clip Speed: %f", &editableSpeed, 0.0f, 2.0f, "%.2f"))
        {
            resource->SetClipSpeed(selectedNode->GetClipName(), editableSpeed);
        }
        if (ImGui::Checkbox("Loop", &loopBuffer))
        {
            if (loopBuffer != clip->loop)
            {
                resource->EditClipInfo(
                    clip->clipName.GetString(), clip->animationResourceUID, clip->clipName.GetString(), loopBuffer,
                clip->animationSpeed);
            }
        }
    }
    else
    {
        ImGui::TextColored(
            ImVec4(1, 0.5f, 0.5f, 1.0f), "No Clip found with name: %s", selectedNode->GetClipName().c_str()
        );
    }

    ImGui::Spacing();
    ImGui::Text("Connected Transitions");
    ImGui::Separator();
    int cont = 0;
    for (auto& transition : resource->transitions)
    {
        bool modified = false;
        char triggerBuffer[64];
        strncpy_s(triggerBuffer, transition.triggerName.GetString().c_str(), sizeof(triggerBuffer));
        triggerBuffer[sizeof(triggerBuffer) - 1] = '\0';

        if (transition.fromState.GetString() == selectedNode->GetStateName())
        {
            ImGui::Text(
                "-> %s (Trigger: %s, Blend: %u ms)", transition.toState.GetString().c_str(),
                transition.triggerName.GetString().c_str(), transition.interpolationTime
            );

            ImGui::PushID(cont);
            int tempInterpolationTime = static_cast<int>(transition.interpolationTime);
            if (ImGui::InputInt("##BlendTime", &tempInterpolationTime))
            {
                transition.interpolationTime =
                    static_cast<uint32_t>(std::max(0, tempInterpolationTime)); // Evitar negativos
                modified = true;
            }

            // Trigger
            int currentTriggerIndex = -1;
            for (size_t i = 0; i < resource->availableTriggers.size(); ++i)
            {
                if (resource->availableTriggers[i] == transition.triggerName.GetString())
                {
                    currentTriggerIndex = static_cast<int>(i);
                    break;
                }
            }

            if (ImGui::Combo(
                    "##TriggerName", &currentTriggerIndex,
                    [](void* data, int idx, const char** out_text)
                    {
                        auto* triggers = static_cast<std::vector<std::string>*>(data);
                        if (out_text) *out_text = (*triggers)[idx].c_str();
                        return true;
                    },
                    &resource->availableTriggers, (int)resource->availableTriggers.size()
                ))
            {
                if (currentTriggerIndex >= 0 &&
                    resource->availableTriggers[currentTriggerIndex] != transition.triggerName.GetString())
                {
                    transition.triggerName = resource->availableTriggers[currentTriggerIndex];
                    modified               = true;
                }
            }
            ImGui::PopID();

            if (modified)
            {
                resource->EditTransition(
                    transition.fromState.GetString(), transition.toState.GetString(),
                    transition.triggerName.GetString(), transition.interpolationTime
                );
            }
        }
        else if (transition.toState.GetString() == selectedNode->GetStateName())
        {
            ImGui::Text(
                "<- %s (Trigger: %s, Blend: %u ms)", transition.fromState.GetString().c_str(),
                transition.triggerName.GetString().c_str(), transition.interpolationTime
            );

            ImGui::PushID(cont);
            int tempInterpolationTime = static_cast<int>(transition.interpolationTime);
            if (ImGui::InputInt("##BlendTime", &tempInterpolationTime))
            {
                transition.interpolationTime =
                    static_cast<uint32_t>(std::max(0, tempInterpolationTime)); // Evitar negativos
                modified = true;
            }

            // Trigger
            int currentTriggerIndex = -1;
            for (size_t i = 0; i < resource->availableTriggers.size(); ++i)
            {
                if (resource->availableTriggers[i] == transition.triggerName.GetString())
                {
                    currentTriggerIndex = static_cast<int>(i);
                    break;
                }
            }

            if (ImGui::Combo(
                    "##TriggerName", &currentTriggerIndex,
                    [](void* data, int idx, const char** out_text)
                    {
                        auto* triggers = static_cast<std::vector<std::string>*>(data);
                        if (out_text) *out_text = (*triggers)[idx].c_str();
                        return true;
                    },
                    &resource->availableTriggers, (int)resource->availableTriggers.size()
                ))
            {
                if (currentTriggerIndex >= 0 &&
                    resource->availableTriggers[currentTriggerIndex] != transition.triggerName.GetString())
                {
                    transition.triggerName = resource->availableTriggers[currentTriggerIndex];
                    modified               = true;
                }
            }
            ImGui::PopID();

            if (modified)
            {
                resource->EditTransition(
                    transition.fromState.GetString(), transition.toState.GetString(),
                    transition.triggerName.GetString(), transition.interpolationTime
                );
            }
        }

        cont++;
    }

    ImGui::End();
}

void StateMachineEditor::BuildGraph()
{
    selectedNode = nullptr;
    if (graph)
    {
        for (const auto& [uid, node] : graph->getNodes())
        {
            if (node)
            {
                for (const auto& pin : node->getIns())
                {
                    pin->deleteLink();
                }

                for (const auto& pin : node->getOuts())
                {
                    pin->deleteLink();
                }
            }
        }
        graph->getNodes().clear();
    }
    graph = std::make_unique<ImFlow::ImNodeFlow>("StateMachineGraph_" + std::to_string(uid));

    std::unordered_map<std::string, std::shared_ptr<StateNode>> stateNodes;
    stateNodes.clear();

    for (const auto& state : resource->states)
    {
        ImVec2 position = state.position;

        auto newNode    = graph->placeNodeAt<StateNode>(position);
        if (newNode)
        {
            auto stateNode = std::dynamic_pointer_cast<StateNode>(newNode);
            if (stateNode)
            {
                availableClips.push_back(state.clipName.GetString());
                stateNode->SetStateName(state.name.GetString());
                stateNode->SetClipName(state.clipName.GetString());

                stateNodes[state.name.GetString()] = stateNode;
            }
        }
        stateCont++;
    }
    graph->update();
    for (const auto& transition : resource->transitions)
    {
        auto itFrom = stateNodes.find(transition.fromState.GetString());
        auto itTo   = stateNodes.find(transition.toState.GetString());

        if (itFrom != stateNodes.end() && itTo != stateNodes.end())
        {
            auto fromNode = itFrom->second;
            auto toNode   = itTo->second;

            if (fromNode && toNode)
            {
                auto outputPin = fromNode->getOutputPin();
                auto inputPin  = toNode->getInputPin();

                if (outputPin && inputPin)
                {
                    // auto link = std::make_shared<ImFlow::Link>(outputPin.get(), inputPin.get(), graph.get());
                    // graph->addLink(link);
                    outputPin->createLink(inputPin.get());
                    // inputPin->connect(link.get());
                    //  resource->availableTriggers.push_back(transition.triggerName.GetString());
                }
                else
                {
                    GLOG("Error: One of the nodes has no valid pins.");
                }
            }
        }
    }

    //GLOG("Total links in graph after adding them: %d", graph->getLinks().size());
    resource->SetDefaultState(0);
}

void StateMachineEditor::DetectNewTransitions()
{
    for (const auto& linkWeak : graph->getLinks())
    {
        if (const auto link = linkWeak.lock())
        {
            ImFlow::Pin* startPin = link->left();
            ImFlow::Pin* endPin   = link->right();

            if (!startPin || !endPin) continue;

            const auto startNode = dynamic_cast<StateNode*>(startPin->getParent());
            const auto endNode   = dynamic_cast<StateNode*>(endPin->getParent());

            if (startNode && endNode)
            {
                std::string fromState = startNode->GetStateName();
                std::string toState   = endNode->GetStateName();

                if (!resource->GetTransition(fromState, toState))
                {
                    //GLOG("Creating transition from %s to %s", fromState.c_str(), toState.c_str());
                    resource->AddTransition(fromState, toState, "Trigger", 200);
                    // availableTriggers.push_back("Trigger");
                }
            }
        }
    }
}

void StateMachineEditor::CreateBaseState(StateNode& node)
{
    State newState;
    std::string stateName = "NewState_" + std::to_string(stateCont);
    std::string clipName  = "Clip_" + std::to_string(resource->clips.size());

    newState.name         = HashString(stateName);
    newState.clipName     = HashString(clipName);

    resource->AddClip(0, clipName, false);
    availableClips.push_back(clipName);
    resource->AddState(newState.name.GetString(), newState.clipName.GetString());
    node.SetStateName(stateName);
    node.SetClipName(clipName);
    stateCont++;
}

void StateMachineEditor::SaveMachine()
{
    ImGui::OpenPopup("Save State Machine");
}

void StateMachineEditor::ShowSavePopup()
{

    if (ImGui::BeginPopupModal("Save State Machine", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        if (!saveInitialized)
        {
            allStateMachineNames = StateMachineManager::GetAllStateMachineNames();

            // Save all node positions
            for (auto& state : resource->states)
            {
                for (const auto& [uid, node] : graph->getNodes())
                {
                    if (node->getName() == state.name.GetString())
                    {
                        state.position = node->getPos();
                    }
                }
            }

            for (const std::string& name : allStateMachineNames)
            {
                if (resource->GetName() == name)
                {
                    strncpy_s(stateMachineName, sizeof(stateMachineName), name.c_str(), _TRUNCATE);
                    alreadySaved = true;
                    break;
                }
            }
            saveInitialized = true;
        }

        ImGui::Text("Enter the name for the State Machine:");
        ImGui::InputText("##StateMachineName", stateMachineName, IM_ARRAYSIZE(stateMachineName));

        if (ImGui::Button("Save"))
        {
            if (std::string(stateMachineName) != resource->GetName())
            {
                alreadySaved = false;
            }
            if (strlen(stateMachineName) > 0)
            {
                resource->SetName(std::string(stateMachineName));
                StateMachineManager::SaveStateMachine(resource, alreadySaved);
                ImGui::CloseCurrentPopup();
                saveInitialized = false;
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
        {
            ImGui::CloseCurrentPopup();
            saveInitialized = false;
        }
        ImGui::EndPopup();
    }
}

void StateMachineEditor::LoadMachine()
{
    availableClips.clear();
    ImGui::OpenPopup("Load State Machine");
}

void StateMachineEditor::ShowLoadPopup()
{

    if (ImGui::BeginPopupModal("Load State Machine", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        if (allStateMachineNames.empty())
        {
            allStateMachineNames = StateMachineManager::GetAllStateMachineNames();
        }
        ImGui::Text("Select a State Machine to Load:");
        ImGui::Separator();

        if (ImGui::BeginListBox("##StateMachineList", ImVec2(-FLT_MIN, 10 * ImGui::GetTextLineHeightWithSpacing())))
        {
            for (size_t i = 0; i < allStateMachineNames.size(); i++)
            {
                if (ImGui::Selectable(allStateMachineNames[i].c_str(), selectedIndex == (int)i))
                {
                    selectedIndex = (int)i;
                }
            }
            ImGui::EndListBox();
        }

        if (ImGui::Button("Load") && selectedIndex >= 0)
        {
            App->GetResourcesModule()->ReleaseResource(resource);
            std::string selectedName = allStateMachineNames[selectedIndex];
            UID selectedUID          = StateMachineManager::GetStateMachineUID(selectedName);

            if (selectedUID != 0)
            {
                //GLOG("Loading State Machine: %s", selectedName.c_str());

                ResourceStateMachine* stateMachine =
                    (ResourceStateMachine*)App->GetResourcesModule()->RequestResource(selectedUID);
                if (stateMachine)
                {
                    //GLOG("Successfully loaded State Machine: %s", selectedName.c_str());
                    resource = stateMachine;
                    BuildGraph();
                    ImGui::CloseCurrentPopup();
                }
                else
                {
                    GLOG("Failed to load State Machine: %s", selectedName.c_str());
                }
            }
        }
        ImGui::SameLine();

        if (ImGui::Button("Cancel"))
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void StateMachineEditor::RemoveStateNode(StateNode& node)
{
    DeleteStateResource(node);

    auto& nodes = graph->getNodes();

    auto it     = nodes.find(node.getUID());
    if (it != nodes.end())
    {
        nodes.erase(it);
    }

    selectedNode = nullptr;
}

void StateMachineEditor::DeleteStateResource(StateNode& node)
{
    const std::string& stateName = node.GetStateName();
    std::string clipName;

    for (const auto& transition : resource->transitions)
    {
        if (transition.fromState.GetString() == stateName || transition.toState.GetString() == stateName)
        {
            resource->RemoveTransition(transition.fromState.GetString(), transition.toState.GetString());
        }
    }

    const State* state = resource->GetState(stateName);
    clipName           = state->clipName.GetString();
    resource->RemoveState(state->name.GetString());

    const Clip* clip = resource->GetClip(clipName);
    resource->RemoveClip(clip->clipName.GetString());
}

void StateMachineEditor::ShowTriggers()
{
    ImGui::OpenPopup("Available Triggers");
}

void StateMachineEditor::ShowTriggersPopup()
{

    if (ImGui::BeginPopupModal("Available Triggers", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Add a new trigger:");

        ImGui::InputText("##NewTriggerInput", newTriggerName, IM_ARRAYSIZE(newTriggerName));

        if (ImGui::Button("Add Trigger"))
        {
            if (strlen(newTriggerName) > 0)
            {
                const bool alreadyExists =
                    std::find(resource->availableTriggers.begin(), resource->availableTriggers.end(), newTriggerName) !=
                    resource->availableTriggers.end();
                if (!alreadyExists)
                {
                    resource->availableTriggers.push_back(newTriggerName);
                    strcpy_s(newTriggerName, "");
                }
            }
        }

        ImGui::Separator();
        ImGui::Text("Available Triggers:");

        for (const std::string& trigger : resource->availableTriggers)
        {
            ImGui::Button(trigger.c_str());
        }

        ImGui::Separator();

        if (ImGui::Button("Cancel"))
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}
