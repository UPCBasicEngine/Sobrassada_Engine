#include "EditorUIModule.h"

#include "Application.h"
#include "CameraModule.h"
#include "Component.h"
#include "EngineEditorBase.h"
#include "FileSystem.h"
#include "GameTimer.h"
#include "InputModule.h"
#include "LibraryModule.h"
#include "OpenGLModule.h"
#include "PathfinderModule.h"
#include "PhysicsModule.h"
#include "ProjectModule.h"
#include "ResourceNavmesh.h"

#include "GameObject.h"
#include "ResourceStateMachine.h"
#include "ResourcesModule.h"
#include "SceneImporter.h"

#include "SceneModule.h"
#include "Script.h"
#include "ScriptModule.h"
#include "StateMachineEditor.h"
#include "TextureEditor.h"
#include "TextureImporter.h"
#include "WindowModule.h"

#include "Math/Quat.h"
#include "SDL.h"
#include "glew.h"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl2.h"
#include "imgui_internal.h"
// imguizmo include after imgui
#include "ImGuizmo.h"
#include <cstring>
#include <filesystem>
#include <string>

EditorUIModule::EditorUIModule() : width(0), height(0)
{
    standaloneComponents = {
        {HashString("Mesh"),                 COMPONENT_MESH                },
        {HashString("Point Light"),          COMPONENT_POINT_LIGHT         },
        {HashString("Spot Light"),           COMPONENT_SPOT_LIGHT          },
        {HashString("Directional Light"),    COMPONENT_DIRECTIONAL_LIGHT   },
        {HashString("Character Controller"), COMPONENT_CHARACTER_CONTROLLER},
        {HashString("Animation"),            COMPONENT_ANIMATION           },
        {HashString("Transform 2D"),         COMPONENT_TRANSFORM_2D        },
        {HashString("UI Canvas"),            COMPONENT_CANVAS              },
        {HashString("UI Label"),             COMPONENT_LABEL               },
        {HashString("Camera"),               COMPONENT_CAMERA              },
        {HashString("Cube Collider"),        COMPONENT_CUBE_COLLIDER       },
        {HashString("Sphere Collider"),      COMPONENT_SPHERE_COLLIDER     },
        {HashString("Capsule Collider"),     COMPONENT_CAPSULE_COLLIDER    },
        {HashString("Script"),               COMPONENT_SCRIPT              },
        {HashString("AI Agent"),             COMPONENT_AIAGENT             },
        {HashString("UI Image"),             COMPONENT_IMAGE               },
        {HashString("UI Button"),            COMPONENT_BUTTON              },
        {HashString("Audio Source"),         COMPONENT_AUDIO_SOURCE        },
        {HashString("Audio Listener"),       COMPONENT_AUDIO_LISTENER      },
    };

    fullscreen    = FULLSCREEN;
    full_desktop  = FULL_DESKTOP;
    borderless    = BORDERLESS;
    resizable     = RESIZABLE;
    frontFaceMode = GL_CCW;
    vsync         = VSYNC;
}

EditorUIModule::~EditorUIModule()
{
    for (auto values : openEditors)
    {
        delete values.second;
    }
    openEditors.clear();
    standaloneComponents.clear();
}

bool EditorUIModule::Init()
{
    context = ImGui::CreateContext();
    ImGuizmo::SetImGuiContext(context);
    ImGuiIO& io     = ImGui::GetIO();
    io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;      // IF using Docking Branch

    ImGui_ImplSDL2_InitForOpenGL(App->GetWindowModule()->window, App->GetOpenGLModule()->GetContext());
    ImGui_ImplOpenGL3_Init("#version 460");

    width                 = App->GetWindowModule()->GetWidth();
    height                = App->GetWindowModule()->GetHeight();

    scenesPath            = App->GetProjectModule()->GetLoadedProjectPath() + SCENES_PATH;
    fileDialogCurrentPath = App->GetProjectModule()->GetLoadedProjectPath();

    return true;
}

void EditorUIModule::DrawScriptInspector(std::function<void()> callback)
{
    ImGui::SetNextItemOpen(true, ImGuiCond_Always);
    if (ImGui::CollapsingHeader("Script Inspector", ImGuiTreeNodeFlags_None))
    {
        callback();
    }
}

update_status EditorUIModule::PreUpdate(float deltaTime)
{
#ifndef GAME

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
    ImGuizmo::BeginFrame();
    // ImGuizmo::SetOrthographic(false);
    // ImGuizmo::AllowAxisFlip(false);
    // ImGuizmo::SetPlaneLimit(0);

    ImGui::DockSpaceOverViewport();

#endif

    return UPDATE_CONTINUE;
}

update_status EditorUIModule::Update(float deltaTime)
{
#ifndef GAME

    if (App->GetProjectModule()->IsProjectLoaded())
    {
        UpdateGizmoTransformMode();
        AddFramePlotData(deltaTime);
        UpdateGizmoDragState();
    }

#endif

    return UPDATE_CONTINUE;
}

update_status EditorUIModule::RenderEditor(float deltaTime)
{
#ifndef GAME
    if (App->GetProjectModule()->IsProjectLoaded())
    {
        Draw();
        for (auto it = openEditors.cbegin(); it != openEditors.cend();)
        {

            if (!it->second->RenderEditor())
            {
                delete it->second;
                it = openEditors.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

#endif
    return UPDATE_CONTINUE;
}

update_status EditorUIModule::PostUpdate(float deltaTime)
{
#ifndef GAME

    if (closeApplication) return UPDATE_STOP;

#endif

    return UPDATE_CONTINUE;
}

bool EditorUIModule::ShutDown()
{
    App->GetScriptModule()->close();

    framerate.clear();
    frametime.clear();

    return true;
}

void EditorUIModule::UpdateGizmoTransformMode()
{
    if (App->GetSceneModule()->GetDoInputsScene())
    {
        const KeyState* keyboard = App->GetInputModule()->GetKeyboard();

        if (keyboard[SDL_SCANCODE_G]) currentGizmoOperation = GizmoOperation::TRANSLATE;
        else if (keyboard[SDL_SCANCODE_H]) currentGizmoOperation = GizmoOperation::ROTATE;
        else if (keyboard[SDL_SCANCODE_J]) currentGizmoOperation = GizmoOperation::SCALE;

        if (keyboard[SDL_SCANCODE_R]) transformType = GizmoTransform::LOCAL;
        else if (keyboard[SDL_SCANCODE_T]) transformType = GizmoTransform::WORLD;
    }
}

ImGuizmo::OPERATION EditorUIModule::GetImGuizmoOperation() const
{
    switch (currentGizmoOperation)
    {
    case GizmoOperation::TRANSLATE:
        return ImGuizmo::TRANSLATE;
    case GizmoOperation::ROTATE:
        return ImGuizmo::ROTATE;
    case GizmoOperation::SCALE:
        return ImGuizmo::SCALE;
    }
    return ImGuizmo::TRANSLATE;
}

ImGuizmo::MODE EditorUIModule::GetImGuizmoTransformMode() const
{
    if (transformType == GizmoTransform::LOCAL) return ImGuizmo::LOCAL;
    else return ImGuizmo::WORLD;
}

void EditorUIModule::AddFramePlotData(float deltaTime)
{
    if (deltaTime == 0) return;

    const float newFrametime = deltaTime * 1000.f;
    const float newFramerate = 1.f / deltaTime;

    if (frametime.size() < maximumPlotData)
    {
        frametime.push_back(newFrametime);
        framerate.push_back(newFramerate);
    }
    else
    {
        frametime.pop_front();
        framerate.pop_front();

        frametime.push_back(newFrametime);
        framerate.push_back(newFramerate);
    }
}

void EditorUIModule::Draw()
{

    MainMenu();
    // ImGui::ShowDemoWindow();

    if (consoleMenu) Console(consoleMenu);

    if (aboutMenu) About(aboutMenu);

    if (importMenu) ImportDialog(importMenu);

    if (loadMenu) LoadDialog(loadMenu);

    if (loadModel) LoadModelDialog(loadModel);

    if (loadPrefab) LoadPrefabDialog(loadPrefab);

    if (saveMenu) SaveDialog(saveMenu);

    if (navmesh) Navmesh(navmesh);

    if (crowdControl) CrowdControl(crowdControl);

    if (editorSettingsMenu) EditorSettings(editorSettingsMenu);
}

void EditorUIModule::MainMenu()
{
    ImGui::BeginMainMenuBar();

    const bool sceneLoaded = App->GetSceneModule()->IsSceneLoaded();
    const bool inPlayMode  = App->GetSceneModule()->GetInPlayMode();

    // File tab menu
    if (ImGui::BeginMenu("File"))
    {
        if (ImGui::MenuItem("Change Project", "")) App->GetProjectModule()->CloseCurrentProject();

        if (ImGui::MenuItem("Create", "")) App->GetSceneModule()->CreateScene();

        if (ImGui::MenuItem("Load", "", loadMenu)) loadMenu = !loadMenu;

        ImGui::BeginDisabled(inPlayMode || !sceneLoaded);
        if (ImGui::MenuItem("Save"))
        {
            if (!App->GetLibraryModule()->SaveScene(scenesPath.c_str(), SaveMode::Save)) saveMenu = !saveMenu;
        }

        if (ImGui::MenuItem("Save as", "", saveMenu)) saveMenu = !saveMenu;
        ImGui::EndDisabled();

        if (ImGui::MenuItem("Quit")) closeApplication = true;

        ImGui::EndMenu();
    }

    // Assets tab menu
    if (ImGui::BeginMenu("Assets"))
    {
        if (ImGui::MenuItem("Import", "", importMenu)) importMenu = !importMenu;

        if (ImGui::BeginMenu("Resource Loader"))
        {
            ImGui::BeginDisabled(!sceneLoaded);
            if (ImGui::MenuItem("Load Model", "", loadModel)) loadModel = !loadModel;

            if (ImGui::MenuItem("Load Prefab", "", loadPrefab)) loadPrefab = !loadPrefab;
            ImGui::EndDisabled();

            ImGui::EndMenu();
        }

        ImGui::EndMenu();
    }

    // View tab menu
    if (ImGui::BeginMenu("View"))
    {
        if (ImGui::MenuItem("Console", "", consoleMenu)) consoleMenu = !consoleMenu;

        if (ImGui::BeginMenu("Scene"))
        {
            ImGui::BeginDisabled(!sceneLoaded);
            if (ImGui::MenuItem("Editor Control", "", editorControlMenu)) editorControlMenu = !editorControlMenu;
            if (ImGui::MenuItem("Hierarchy", "", hierarchyMenu)) hierarchyMenu = !hierarchyMenu;
            if (ImGui::MenuItem("Inspector", "", inspectorMenu)) inspectorMenu = !inspectorMenu;
            if (ImGui::MenuItem("Lights Config", "", lightConfig)) lightConfig = !lightConfig;
            if (ImGui::MenuItem("Navmesh", "", navmesh)) navmesh = !navmesh;
            if (ImGui::MenuItem("Crowd Control", "", crowdControl)) crowdControl = !crowdControl;
            ImGui::EndDisabled();

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Engine Editor Window"))
        {
            if (ImGui::MenuItem("Mockup Base Engine Editor", "")) OpenEditor(CreateEditor(EditorType::BASE));

            if (ImGui::MenuItem("Node Editor", "")) OpenEditor(CreateEditor(EditorType::NODE));

            if (ImGui::MenuItem("State Machine Editor Engine Editor", ""))
                OpenEditor(CreateEditor(EditorType::ANIMATION));
            if (ImGui::MenuItem("Texture Editor Engine Editor", "")) OpenEditor(CreateEditor(EditorType::TEXTURE));

            ImGui::EndMenu();
        }

        if (ImGui::MenuItem("Editor settings", "", editorSettingsMenu)) editorSettingsMenu = !editorSettingsMenu;

        if (ImGui::MenuItem("About", "", aboutMenu)) aboutMenu = !aboutMenu;

        ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
}

void EditorUIModule::LoadDialog(bool& loadMenu)
{
    ImGui::SetNextWindowSize(ImVec2(width * 0.25f, height * 0.4f), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin("Load Scene", &loadMenu, ImGuiWindowFlags_NoCollapse))
    {
        ImGui::End();
        return;
    }

    ImGui::BeginChild("scrollFiles", ImVec2(0, -30), ImGuiChildFlags_Borders);

    if (FileSystem::Exists(scenesPath.c_str()))
    {
        if (ImGui::TreeNodeEx("Scenes", ImGuiTreeNodeFlags_DefaultOpen))
        {
            FileSystem::GetFilesSorted(scenesPath, filesLoad);

            for (int i = 0; i < filesLoad.size(); i++)
            {
                const std::string& file = filesLoad[i];
                if (ImGui::Selectable(file.c_str(), selectedLoad == i))
                {
                    selectedLoad  = i;
                    inputFileLoad = file;
                }
            }

            ImGui::TreePop();
        }
    }

    ImGui::EndChild();

    ImGui::InputText("##filename", &inputFileLoad[0], inputFileLoad.size(), ImGuiInputTextFlags_ReadOnly);

    ImGui::SameLine();

    if (ImGui::Button("Ok", ImVec2(0, 0)))
    {
        if (!inputFileLoad.empty()) App->GetLibraryModule()->LoadScene(inputFileLoad.c_str());

        loadMenu = false;
    }

    ImGui::SameLine();

    if (ImGui::Button("Cancel", ImVec2(0, 0))) loadMenu = false;

    ImGui::End();

    if (!loadMenu)
    {
        filesLoad.clear();
        filesLoad.shrink_to_fit();
        selectedLoad  = -1;
        inputFileLoad = "";
    }
}

void EditorUIModule::Navmesh(bool& navmesh)
{
    if (!navmesh) return;

    ImGui::Begin("NavMesh Creation", &navmesh, ImGuiWindowFlags_None);

    // Draw config UI
    App->GetPathfinderModule()->GetNavMeshConfig().RenderEditorUI();

    ImGui::InputText("NavMesh Name", navmeshName, IM_ARRAYSIZE(navmeshName));

    // Create navmesh
    if (ImGui::Button("Create NavMesh"))
    {
        App->GetPathfinderModule()->CreateNavMesh();
        ImGui::Text("NavMesh created!");
    }

    // Save navmesh
    if (ImGui::Button("Save NavMesh"))
    {
        App->GetPathfinderModule()->SaveNavMesh(navmeshName); // or ask user for name
        ImGui::Text("NavMesh saved!");
    }

    // Load navmesh
    if (ImGui::Button("Load NavMesh"))
    {
        showNavLoadDialog = true;
    }

    ImGui::End(); // End NavMesh Creation

    // Load dialog window
    if (showNavLoadDialog)
    {
        ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
        bool open = true;
        if (ImGui::Begin("Load NavMesh", &open, ImGuiWindowFlags_NoCollapse))
        {
            ImGui::InputText("Search", searchTextNavmesh, IM_ARRAYSIZE(searchTextNavmesh));
            ImGui::Separator();

            if (ImGui::BeginListBox("##NavmeshList", ImVec2(-FLT_MIN, -40)))
            {
                int i = 0;
                for (const auto& pair : App->GetLibraryModule()->GetNavmeshMap())
                {
                    if (pair.first.GetString().find(searchTextNavmesh) != std::string::npos)
                    {
                        ++i;
                        if (ImGui::Selectable(pair.first.c_str(), selectedNavmesh == i))
                        {
                            selectedNavmesh = i;
                            navmeshUID      = pair.second;
                        }
                    }
                }
                ImGui::EndListBox();
            }

            ImGui::Dummy(ImVec2(0, 3));

            if (ImGui::Button("Load"))
            {
                if (navmeshUID != INVALID_UID)
                {
                    App->GetPathfinderModule()->LoadNavMesh(App->GetLibraryModule()->GetResourceName(navmeshUID));
                }
                open = false;
            }

            ImGui::SameLine();

            if (ImGui::Button("Cancel"))
            {
                open = false;
            }

            ImGui::End();
        }

        if (!open)
        {
            showNavLoadDialog    = false;

            // Reset search and selection
            searchTextNavmesh[0] = '\0';
            selectedNavmesh      = -1;
            navmeshUID           = INVALID_UID;
        }
    }
}

void EditorUIModule::CrowdControl(bool& crowdControl)
{
    if (!crowdControl) return;

    ImGui::Begin("Crowd Control", &crowdControl, ImGuiWindowFlags_None);

    App->GetPathfinderModule()->RenderCrowdEditor();

    ImGui::End();
}

void EditorUIModule::LoadPrefabDialog(bool& loadPrefab)
{
    ImGui::SetNextWindowSize(ImVec2(width * 0.25f, height * 0.4f), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin("Load Prefab", &loadPrefab, ImGuiWindowFlags_NoCollapse))
    {
        ImGui::End();
        return;
    }

    ImGui::InputText("Search", searchTextPrefab, IM_ARRAYSIZE(searchTextPrefab));

    ImGui::Separator();
    if (ImGui::BeginListBox("##PrefabsList", ImVec2(-FLT_MIN, -65)))
    {
        int i = 0;
        for (const auto& valuePair : App->GetLibraryModule()->GetPrefabMap())
        {
            ++i;
            if (valuePair.first.GetString().find(searchTextPrefab) != std::string::npos)
            {
                if (ImGui::Selectable(valuePair.first.c_str(), selectedPrefab == i))
                {
                    selectedPrefab = i;
                    prefabUID      = valuePair.second;
                }
            }
        }
        ImGui::EndListBox();
    }

    ImGui::Dummy(ImVec2(0, 3));

    if (ImGui::Button("Ok", ImVec2(0, 0))) App->GetSceneModule()->GetScene()->LoadPrefab(prefabUID);

    ImGui::SameLine();

    if (ImGui::Button("Cancel", ImVec2(0, 0))) loadPrefab = false;

    ImGui::Dummy(ImVec2(0, 5));

    if (ImGui::Button("DELETE!", ImVec2(0, 0))) App->GetLibraryModule()->DeletePrefabFiles(prefabUID);

    ImGui::End();

    if (!loadPrefab) searchTextPrefab[0] = '\0';
}

void EditorUIModule::LoadModelDialog(bool& loadModel)
{
    ImGui::SetNextWindowSize(ImVec2(width * 0.25f, height * 0.4f), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin("Load Model", &loadModel, ImGuiWindowFlags_NoCollapse))
    {
        ImGui::End();
        return;
    }

    ImGui::InputText("Search", searchTextModel, IM_ARRAYSIZE(searchTextModel));

    ImGui::Separator();
    if (ImGui::BeginListBox("##ModelsList", ImVec2(-FLT_MIN, -40)))
    {
        int i = 0;
        for (const auto& valuePair : App->GetLibraryModule()->GetModelMap())
        {
            ++i;
            if (valuePair.first.GetString().find(searchTextModel) != std::string::npos)
            {
                if (ImGui::Selectable(valuePair.first.c_str(), selectedModel == i))
                {
                    selectedModel = i;
                    modelUID      = valuePair.second;
                }
            }
        }
        ImGui::EndListBox();
    }

    ImGui::Dummy(ImVec2(0, 3));

    if (ImGui::Button("Ok", ImVec2(0, 0))) App->GetSceneModule()->GetScene()->LoadModel(modelUID);

    ImGui::SameLine();

    if (ImGui::Button("Cancel", ImVec2(0, 0))) loadModel = false;

    ImGui::End();

    if (!loadModel) searchTextModel[0] = '\0';
}

void EditorUIModule::SaveDialog(bool& saveMenu)
{
    ImGui::SetNextWindowSize(ImVec2(width * 0.25f, height * 0.4f), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin("Save Scene", &saveMenu, ImGuiWindowFlags_NoCollapse))
    {
        ImGui::End();
        return;
    }

    ImGui::BeginChild("scrollFiles", ImVec2(0, -30), ImGuiChildFlags_Borders);

    if (FileSystem::Exists(scenesPath.c_str()))
    {
        if (ImGui::TreeNodeEx("Scenes", ImGuiTreeNodeFlags_DefaultOpen))
        {
            FileSystem::GetFilesSorted(scenesPath, filesSave);

            for (int i = 0; i < filesSave.size(); i++)
            {
                const std::string& file = filesSave[i];
                if (ImGui::Selectable(file.c_str()))
                {
                }
            }

            ImGui::TreePop();
        }
    }

    ImGui::EndChild();

    ImGui::InputText("##filename", inputFileSave, IM_ARRAYSIZE(inputFileSave));

    ImGui::SameLine();

    if (ImGui::Button("Ok", ImVec2(0, 0)))
    {
        if (strlen(inputFileSave) > 0)
        {
            std::string savePath = scenesPath + inputFileSave + SCENE_EXTENSION;
            App->GetLibraryModule()->SaveScene(savePath.c_str(), SaveMode::SaveAs);
        }
        saveMenu = false;
    }

    ImGui::SameLine();

    if (ImGui::Button("Cancel", ImVec2(0, 0))) saveMenu = false;

    ImGui::End();

    if (!saveMenu)
    {
        filesSave.clear();
        filesSave.shrink_to_fit();
        inputFileSave[0] = '\0';
    }
}

std::string EditorUIModule::RenderFileDialog(bool& window, const char* windowTitle, bool selectFolder)
{
    ImGui::SetNextWindowSize(ImVec2(width * 0.4f, height * 0.4f), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin(windowTitle, &window, ImGuiWindowFlags_NoCollapse))
    {
        ImGui::End();
        return "";
    }

    if (fileDialogCurrentPath == "") fileDialogCurrentPath = "C:";

    if (ImGui::Button("Drives"))
    {
        FileSystem::GetDrives(filesFileDialog);
        showDrives = true;
    }

    ImGui::SameLine();
    ImGui::Text("|");

    if (!showDrives)
    {
        ImGui::SameLine();

        if (loadButtons)
        {
            FileSystem::SplitAccumulatedPath(fileDialogCurrentPath, accPaths);
            loadButtons = false;
        }

        for (size_t i = 0; i < accPaths.size(); i++)
        {
            const std::string& accPath     = accPaths[i];

            const std::string& buttonLabel = (i == 0) ? accPath : FileSystem::GetFileNameWithExtension(accPath);

            if (ImGui::Button(buttonLabel.c_str()))
            {
                fileDialogCurrentPath    = accPath;
                showDrives               = false;
                doLoadFiles              = true;
                loadButtons              = true;
                searchQueryFileDialog[0] = '\0';
                FileSystem::SplitAccumulatedPath(fileDialogCurrentPath, accPaths);
            }

            if (i < accPaths.size() - 1) ImGui::SameLine();
        }
    }

    ImGui::Separator();

    ImGui::Text("Search:");
    ImGui::SameLine();
    ImGui::InputText("##search", searchQueryFileDialog, IM_ARRAYSIZE(searchQueryFileDialog));

    ImGui::BeginChild("scrollFiles", ImVec2(0, -70), ImGuiChildFlags_Borders);

    if (showDrives)
    {
        for (const std::string& drive : filesFileDialog)
        {
            if (ImGui::Selectable(drive.c_str()))
            {
                fileDialogCurrentPath = drive;
                showDrives            = false;
                doLoadFiles           = true;
                loadButtons           = true;
            }
        }
    }
    else
    {
        // root directory add delimiter
        if (fileDialogCurrentPath.back() == ':') fileDialogCurrentPath += DELIMITER;

        if (filesFileDialog.empty() || doLoadFiles)
        {
            FileSystem::GetFilesSorted(fileDialogCurrentPath, filesFileDialog);

            if (accPaths.size() > 1) filesFileDialog.insert(filesFileDialog.begin(), "..");

            doLoadFiles       = false;
            loadFilteredFiles = true;
        }

        if (strcmp(lastQueryFileDialog, searchQueryFileDialog) != 0 ||
            loadFilteredFiles) // if the search query has changed
        {
            strcpy_s(lastQueryFileDialog, searchQueryFileDialog);

            filteredFiles.clear();

            for (const std::string& file : filesFileDialog)
            {
                if (file.find(searchQueryFileDialog) != std::string::npos)
                {
                    filteredFiles.push_back(file);
                }
            }

            loadFilteredFiles = false;
        }

        for (int i = 0; i < filteredFiles.size(); i++)
        {
            const std::string& file = filteredFiles[i];
            std::string filePath    = fileDialogCurrentPath + DELIMITER + file;
            bool isDirectory        = FileSystem::IsDirectory(filePath.c_str());

            std::string tag         = isDirectory ? "[Dir] " : "[File] ";
            std::string fileWithTag = tag + file;

            if (ImGui::Selectable(fileWithTag.c_str(), selectedFileDialog == i))
            {
                selectedFileDialog = i;

                if (file == "..")
                {
                    fileDialogCurrentPath = FileSystem::GetParentPath(fileDialogCurrentPath);
                    inputFileDialog       = "";
                    selectedFileDialog    = -1;
                    FileSystem::GetFilesSorted(fileDialogCurrentPath, filesFileDialog);
                    searchQueryFileDialog[0] = '\0';
                    doLoadFiles              = true;
                    loadButtons              = true;
                }
                else if (isDirectory && !selectFolder)
                {
                    fileDialogCurrentPath = filePath;
                    inputFileDialog       = "";
                    selectedFileDialog    = -1;
                    FileSystem::GetFilesSorted(fileDialogCurrentPath, filesFileDialog);
                    searchQueryFileDialog[0] = '\0';
                    doLoadFiles              = true;
                    loadButtons              = true;
                }
                else if (isDirectory && selectFolder)
                {
                    fileDialogCurrentPath = filePath;
                    inputFileDialog       = FileSystem::GetFileNameWithExtension(file);
                    FileSystem::GetFilesSorted(fileDialogCurrentPath, filesFileDialog);
                    searchQueryFileDialog[0] = '\0';
                    doLoadFiles              = true;
                    loadButtons              = true;
                }
                else
                {
                    inputFileDialog = FileSystem::GetFileNameWithExtension(file);
                }
            }
        }
    }

    ImGui::EndChild();

    ImGui::Dummy(ImVec2(0, 10));
    ImGui::Text("File Name:");
    ImGui::SameLine();
    ImGui::InputText("##filename", &inputFileDialog[0], inputFileDialog.size(), ImGuiInputTextFlags_ReadOnly);

    std::string importPath = "";
    if (ImGui::Button("Ok", ImVec2(0, 0)))
    {
        if (!inputFileDialog.empty())
        {
            if (selectFolder) importPath = fileDialogCurrentPath;
            else importPath = fileDialogCurrentPath + DELIMITER + inputFileDialog;
        }

        window = false;
    }

    ImGui::SameLine();

    if (ImGui::Button("Cancel", ImVec2(0, 0))) window = false;

    ImGui::End();

    if (!window)
    {
        accPaths.clear();
        accPaths.shrink_to_fit();
        filesFileDialog.clear();
        filesFileDialog.shrink_to_fit();
        filteredFiles.clear();
        filteredFiles.shrink_to_fit();
        showDrives               = false;
        inputFileDialog          = "";
        searchQueryFileDialog[0] = '\0';
        doLoadFiles              = true;
        loadButtons              = true;
    }

    return importPath;
}

void EditorUIModule::DrawScriptInspector(const std::vector<InspectorField>& fields)
{

    for (auto& field : fields)
    {
        switch (field.type)
        {
        case InspectorField::FieldType::Text:
            ImGui::Text(static_cast<const char*>(field.data));
            break;
        case InspectorField::FieldType::Float:
            ImGui::SliderFloat(field.name, (float*)field.data, field.minValue, field.maxValue);
            break;
        case InspectorField::FieldType::Bool:
            ImGui::Checkbox(field.name, (bool*)field.data);
            break;
        case InspectorField::FieldType::Int:
            ImGui::InputInt(field.name, (int*)field.data);
            break;
        case InspectorField::FieldType::Vec2:
        {
            float* vec2Data = reinterpret_cast<float*>(field.data);
            ImGui::SliderFloat2(field.name, vec2Data, field.minValue, field.maxValue);
            break;
        }
        case InspectorField::FieldType::Vec3:
        {
            float* vec3Data = reinterpret_cast<float*>(field.data);
            ImGui::SliderFloat3(field.name, vec3Data, field.minValue, field.maxValue);
            break;
        }
        case InspectorField::FieldType::Vec4:
        {
            float* vec4Data = reinterpret_cast<float*>(field.data);
            ImGui::SliderFloat4(field.name, vec4Data, field.minValue, field.maxValue);
            break;
        }
        case InspectorField::FieldType::Color:
        {
            ImColor* color = (ImColor*)field.data;
            ImGui::ColorEdit3(field.name, (float*)&color->Value);
            break;
        }
        case InspectorField::FieldType::InputText:
        {
            // Use InputText with strings (I don't know how this works)
            std::string* str = static_cast<std::string*>(field.data);
            ImGui::InputText(
                field.name, str->data(), str->capacity() + 1, ImGuiInputTextFlags_CallbackResize,
                [](ImGuiInputTextCallbackData* data) -> int
                {
                    if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
                    {
                        std::string* str = static_cast<std::string*>(data->UserData);
                        str->resize(data->BufTextLen);
                    }
                    return 0;
                },
                static_cast<void*>(str)
            );
            break;
        }
        case InspectorField::FieldType::GameObject:
        {
            GameObject** selectedGO = (GameObject**)field.data;
            const char* currentName = (*selectedGO) ? (*selectedGO)->GetName().c_str() : "None";
            if (ImGui::BeginCombo(field.name, currentName))
            {
                if (ImGui::Selectable("None", *selectedGO == nullptr))
                {
                    *selectedGO = nullptr;
                }

                for (const auto& pair : App->GetSceneModule()->GetScene()->GetAllGameObjects())
                {
                    GameObject* go          = pair.second;
                    const std::string& name = go->GetName();

                    if (name == "SceneModule GameObject" || name == "MULTISELECT_DUMMY") continue;

                    std::string label = name + "##" + std::to_string(go->GetUID());
                    bool isSelected   = (*selectedGO == go);
                    if (ImGui::Selectable(label.c_str(), isSelected)) *selectedGO = go;

                    if (isSelected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            break;
        }
        }
    }
}

void EditorUIModule::ImportDialog(bool& import)
{
    const std::string resultingPath = RenderFileDialog(import, "Import Asset");
    if (!resultingPath.empty())
    {
        SceneImporter::Import(resultingPath.c_str());
    }
}

void EditorUIModule::Console(bool& consoleMenu) const
{
    if (!ImGui::Begin("Console", &consoleMenu))
    {
        ImGui::End();
        return;
    }

    int index = 0;
    for (const char* log : *Logs)
    {
        std::string label = std::string(log) + "##" + std::to_string(index);
        ImGui::Selectable(label.c_str());

        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Copy Text"))
            {
                ImGui::SetClipboardText(log);
            }
            ImGui::EndPopup();
        }
        ++index;
    }

    // Autoscroll only if the scroll is in the bottom position
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
    {
        ImGui::SetScrollHereY(1.0f);
    }

    ImGui::End();
}

bool EditorUIModule::RenderTransformWidget(
    float4x4& localTransform, float4x4& globalTransform, const float4x4& parentTransform, float3& pos, float3& rot,
    float3& scale
)
{
    bool positionValueChanged = false, rotationValueChanged = false, scaleValueChanged = false;
    float3 originalScale      = float3(scale);

    std::string transformName = std::string(transformType == GizmoTransform::LOCAL ? "Local " : "World ") + "Transform";
    ImGui::SeparatorText(transformName.c_str());

    if (transformType == GizmoTransform::LOCAL && !pos.Equals(localTransform.TranslatePart()))
    {
        pos   = localTransform.TranslatePart();
        rot   = localTransform.RotatePart().ToEulerXYZ();
        scale = localTransform.GetScale();
    }
    else if (transformType == GizmoTransform::WORLD && !pos.Equals(globalTransform.TranslatePart()))
    {
        pos   = globalTransform.TranslatePart();
        rot   = globalTransform.RotatePart().ToEulerXYZ();
        scale = globalTransform.GetScale();
    }

    RenderBasicTransformModifiers(
        pos, rot, scale, lockScaleAxis, positionValueChanged, rotationValueChanged, scaleValueChanged
    );

    if (positionValueChanged || rotationValueChanged || scaleValueChanged)
    {
        if (scaleValueChanged && lockScaleAxis)
        {
            if (originalScale.IsZero())
            {
                float scaleFactor = 1;
                if (scale.x != originalScale.x) scaleFactor = scale.x;

                else if (scale.y != originalScale.y) scaleFactor = scale.y;

                else if (scale.z != originalScale.z) scaleFactor = scale.z;

                scale = float3(scaleFactor, scaleFactor, scaleFactor);
            }
            else
            {
                float scaleFactor = 1;
                if (scale.x != originalScale.x && scale.x != 0)
                    scaleFactor = originalScale.x == 0 ? 1 : scale.x / originalScale.x;

                else if (scale.y != originalScale.y && scale.y != 0)
                    scaleFactor = originalScale.y == 0 ? 1 : scale.y / originalScale.y;

                else if (scale.z != originalScale.z && scale.z != 0)
                    scaleFactor = originalScale.z == 0 ? 1 : scale.z / originalScale.z;

                originalScale *= scaleFactor;
                scale          = originalScale;
            }
        }

        float4x4 outputTransform = float4x4::FromTRS(pos, Quat::FromEulerXYZ(rot.x, rot.y, rot.z), scale);

        if (transformType == GizmoTransform::WORLD) localTransform = parentTransform.Inverted() * outputTransform;

        else localTransform = outputTransform;
    }

    return positionValueChanged || rotationValueChanged || scaleValueChanged;
}

bool EditorUIModule::RenderImGuizmo(
    float4x4& localTransform, float4x4& globalTransform, const float4x4& parentTransform, float3& pos, float3& rot,
    float3& scale
) const
{
    float4x4 view = float4x4(App->GetCameraModule()->GetViewMatrix());
    view.Transpose();

    float4x4 proj = float4x4(App->GetCameraModule()->GetProjectionMatrix());
    proj.Transpose();

    float maxDistance  = App->GetCameraModule()->GetFarPlaneDistance() * 0.9f;

    float4x4 transform = float4x4(globalTransform);
    transform.Transpose();

    ImGuizmo::OPERATION operation = GetImGuizmoOperation();
    ImGuizmo::MODE mode           = GetImGuizmoTransformMode();

    if (App->GetCameraModule()->GetOrbiting()) ImGuizmo::Enable(false);
    else ImGuizmo::Enable(true);
    ImGuizmo::Manipulate(
        view.ptr(), proj.ptr(), operation, mode, transform.ptr(), nullptr, snapEnabled ? snapValues.ptr() : nullptr,
        nullptr, nullptr
    );

    if (!ImGuizmo::IsUsing()) return false;

    if (App->GetSceneModule()->GetDoInputsScene())
    {
        transform.Transpose();
        if (transform.TranslatePart().Distance(App->GetCameraModule()->GetCameraPosition()) > maxDistance)
        {
            ImGuizmo::Enable(false);
            return false;
        }
        pos            = transform.TranslatePart();
        rot            = transform.RotatePart().ToEulerXYZ();
        scale          = transform.GetScale();
        localTransform = parentTransform.Inverted() * transform;
    }

    return true;
}

template UID EditorUIModule::RenderResourceSelectDialog<UID>(
    const char* id, const std::unordered_map<HashString, UID>& availableResources, const UID& defaultResource
);
template ComponentType EditorUIModule::RenderResourceSelectDialog<ComponentType>(
    const char* id, const std::unordered_map<HashString, ComponentType>& availableResources,
    const ComponentType& defaultResource
);
template uint32_t EditorUIModule::RenderResourceSelectDialog<uint32_t>(
    const char* id, const std::unordered_map<HashString, uint32_t>& availableResources, const uint32_t& defaultResource
);
void EditorUIModule::RenderBasicTransformModifiers(
    float3& outputPosition, float3& outputRotation, float3& outputScale, bool& lockScaleAxis,
    bool& positionValueChanged, bool& rotationValueChanged, bool& scaleValueChanged
)
{
    positionValueChanged |= ImGui::InputFloat3("Position", &outputPosition[0]);

    outputRotation       *= RAD_DEGREE_CONV;

    rotationValueChanged |= ImGui::InputFloat3("Rotation", &outputRotation[0]);

    outputRotation       /= RAD_DEGREE_CONV;

    scaleValueChanged    |= ImGui::InputFloat3("Scale", &outputScale[0]);
    ImGui::SameLine();
    ImGui::Checkbox("Lock axis", &lockScaleAxis);
}

template <typename T>
T EditorUIModule::RenderResourceSelectDialog(
    const char* id, const std::unordered_map<HashString, T>& availableResources, const T& defaultResource
)
{
    T result = defaultResource;
    if (ImGui::BeginPopup(id))
    {
        ImGui::InputText("Search", searchTextResource, IM_ARRAYSIZE(searchTextResource));

        ImGui::Separator();
        if (ImGui::BeginListBox("##ComponentList", ImVec2(-FLT_MIN, 5 * ImGui::GetTextLineHeightWithSpacing())))
        {
            for (const auto& valuePair : availableResources)
            {
                {
                    if (valuePair.first.GetString().find(searchTextResource) != std::string::npos)
                    {
                        if (ImGui::Selectable(valuePair.first.c_str(), false))
                        {
                            result = valuePair.second;
                            memset(searchTextResource, 0, sizeof searchTextResource);
                            ImGui::CloseCurrentPopup();
                        }
                    }
                }
            }
            ImGui::EndListBox();
        }
        ImGui::EndPopup();
    }
    return result;
}

void EditorUIModule::OpenEditor(EngineEditorBase* editorToOpen)
{
    if (editorToOpen != nullptr)
    {
        openEditors.insert({editorToOpen->GetUID(), editorToOpen});
    }
}

void EditorUIModule::About(bool& aboutMenu)
{
    std::string title = "About " + std::string(ENGINE_NAME);
    if (!ImGui::Begin(title.c_str(), &aboutMenu))
    {
        ImGui::End();
        return;
    }

    ImGui::Text("%s (%s)", ENGINE_NAME, ENGINE_VERSION);

    ImGui::TextLinkOpenURL("GitHub", "https://github.com/UPCBasicEngine/Sobrassada_Engine");
    ImGui::SameLine();
    ImGui::TextLinkOpenURL("Readme", "https://github.com/UPCBasicEngine/Sobrassada_Engine/blob/main/README.md");
    ImGui::SameLine();
    ImGui::TextLinkOpenURL("License", "https://github.com/UPCBasicEngine/Sobrassada_Engine/blob/main/LICENSE.md");
    ImGui::SameLine();
    ImGui::TextLinkOpenURL("Releases", "https://github.com/UPCBasicEngine/Sobrassada_Engine/releases");

    ImGui::Separator();
    ImGui::Text("Game engine developed with OpenGL as part of a Video Game Master's Project at UPC.");
    ImGui::Text("Organization: %s", ORGANIZATION_NAME);
    ImGui::Text("Libraries:");
    ImGui::Text(" - Window: SDL v2.0.16");
    ImGui::Text(" - OpenGL: v4.6.0");
    ImGui::Text(" - OpenGL extensions: Glew v2.1.0");
    ImGui::Text(" - Editor: Dear ImGui v1.91.5");
    ImGui::Text(" - Textures: DirectXTex v2.0.6");
    ImGui::Text(" - Geometry loader: TinyGLTF v2.9.3");
    ImGui::Text(" - Math: MathGeoLib v1.5");
    ImGui::Text(" - JSON: rapidjson v1.1");
    ImGui::Text(" - UI: FreeType v2.13.3");
    ImGui::Text(" - NavMesh: RecastNavigation v1.6.0");
    ImGui::Text(" - StateMachine: ImNodeFlow v1.2.2");
    ImGui::Text(" - Physics: Bullet v3.25");
    ImGui::Text(" - Audio: Wwise v2024.1.3.8749");
    ImGui::Text("%s is licensed under the MIT License, see LICENSE for more information.", ENGINE_NAME);

    ImGui::Checkbox("Config/Build Information", &showConfigInfo);
    if (showConfigInfo)
    {
        ImGuiIO& io            = ImGui::GetIO();
        ImGuiStyle& style      = ImGui::GetStyle();

        bool copy_to_clipboard = ImGui::Button("Copy to clipboard");
        ImVec2 child_size      = ImVec2(0, ImGui::GetTextLineHeightWithSpacing() * 18);
        ImGui::BeginChild(ImGui::GetID("cfg_infos"), child_size, ImGuiChildFlags_FrameStyle);
        if (copy_to_clipboard)
        {
            ImGui::LogToClipboard();
            ImGui::LogText("```\n"); // Back quotes will make text appears without formatting when pasting on GitHub
        }

        ImGui::Text("%s (%s)", ENGINE_NAME, ENGINE_VERSION);
        ImGui::Separator();
        ImGui::Text(
            "sizeof(size_t): %d, sizeof(ImDrawIdx): %d, sizeof(ImDrawVert): %d", (int)sizeof(size_t),
            (int)sizeof(ImDrawIdx), (int)sizeof(ImDrawVert)
        );
        ImGui::Text("define: __cplusplus=%d", (int)__cplusplus);
#ifdef IMGUI_DISABLE_OBSOLETE_FUNCTIONS
        ImGui::Text("define: IMGUI_DISABLE_OBSOLETE_FUNCTIONS");
#endif
#ifdef IMGUI_DISABLE_WIN32_DEFAULT_CLIPBOARD_FUNCTIONS
        ImGui::Text("define: IMGUI_DISABLE_WIN32_DEFAULT_CLIPBOARD_FUNCTIONS");
#endif
#ifdef IMGUI_DISABLE_WIN32_DEFAULT_IME_FUNCTIONS
        ImGui::Text("define: IMGUI_DISABLE_WIN32_DEFAULT_IME_FUNCTIONS");
#endif
#ifdef IMGUI_DISABLE_WIN32_FUNCTIONS
        ImGui::Text("define: IMGUI_DISABLE_WIN32_FUNCTIONS");
#endif
#ifdef IMGUI_DISABLE_DEFAULT_FORMAT_FUNCTIONS
        ImGui::Text("define: IMGUI_DISABLE_DEFAULT_FORMAT_FUNCTIONS");
#endif
#ifdef IMGUI_DISABLE_DEFAULT_MATH_FUNCTIONS
        ImGui::Text("define: IMGUI_DISABLE_DEFAULT_MATH_FUNCTIONS");
#endif
#ifdef IMGUI_DISABLE_DEFAULT_FILE_FUNCTIONS
        ImGui::Text("define: IMGUI_DISABLE_DEFAULT_FILE_FUNCTIONS");
#endif
#ifdef IMGUI_DISABLE_FILE_FUNCTIONS
        ImGui::Text("define: IMGUI_DISABLE_FILE_FUNCTIONS");
#endif
#ifdef IMGUI_DISABLE_DEFAULT_ALLOCATORS
        ImGui::Text("define: IMGUI_DISABLE_DEFAULT_ALLOCATORS");
#endif
#ifdef IMGUI_USE_BGRA_PACKED_COLOR
        ImGui::Text("define: IMGUI_USE_BGRA_PACKED_COLOR");
#endif
#ifdef _WIN32
        ImGui::Text("define: _WIN32");
#endif
#ifdef _WIN64
        ImGui::Text("define: _WIN64");
#endif
#ifdef __linux__
        ImGui::Text("define: __linux__");
#endif
#ifdef __APPLE__
        ImGui::Text("define: __APPLE__");
#endif
#ifdef _MSC_VER
        ImGui::Text("define: _MSC_VER=%d", _MSC_VER);
#endif
#ifdef _MSVC_LANG
        ImGui::Text("define: _MSVC_LANG=%d", (int)_MSVC_LANG);
#endif
#ifdef __MINGW32__
        ImGui::Text("define: __MINGW32__");
#endif
#ifdef __MINGW64__
        ImGui::Text("define: __MINGW64__");
#endif
#ifdef __GNUC__
        ImGui::Text("define: __GNUC__=%d", (int)__GNUC__);
#endif
#ifdef __clang_version__
        ImGui::Text("define: __clang_version__=%s", __clang_version__);
#endif
#ifdef __EMSCRIPTEN__
        ImGui::Text("define: __EMSCRIPTEN__");
        ImGui::Text("Emscripten: %d.%d.%d", __EMSCRIPTEN_major__, __EMSCRIPTEN_minor__, __EMSCRIPTEN_tiny__);
#endif
#ifdef IMGUI_HAS_VIEWPORT
        ImGui::Text("define: IMGUI_HAS_VIEWPORT");
#endif
#ifdef IMGUI_HAS_DOCK
        ImGui::Text("define: IMGUI_HAS_DOCK");
#endif
        ImGui::Separator();
        ImGui::Text("io.BackendPlatformName: %s", io.BackendPlatformName ? io.BackendPlatformName : "NULL");
        ImGui::Text("io.BackendRendererName: %s", io.BackendRendererName ? io.BackendRendererName : "NULL");
        ImGui::Text("io.ConfigFlags: 0x%08X", io.ConfigFlags);
        if (io.ConfigFlags & ImGuiConfigFlags_NavEnableKeyboard) ImGui::Text(" NavEnableKeyboard");
        if (io.ConfigFlags & ImGuiConfigFlags_NavEnableGamepad) ImGui::Text(" NavEnableGamepad");
        if (io.ConfigFlags & ImGuiConfigFlags_NoMouse) ImGui::Text(" NoMouse");
        if (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange) ImGui::Text(" NoMouseCursorChange");
        if (io.ConfigFlags & ImGuiConfigFlags_NoKeyboard) ImGui::Text(" NoKeyboard");
        if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) ImGui::Text(" DockingEnable");
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) ImGui::Text(" ViewportsEnable");
        if (io.ConfigFlags & ImGuiConfigFlags_DpiEnableScaleViewports) ImGui::Text(" DpiEnableScaleViewports");
        if (io.ConfigFlags & ImGuiConfigFlags_DpiEnableScaleFonts) ImGui::Text(" DpiEnableScaleFonts");
        if (io.MouseDrawCursor) ImGui::Text("io.MouseDrawCursor");
        if (io.ConfigViewportsNoAutoMerge) ImGui::Text("io.ConfigViewportsNoAutoMerge");
        if (io.ConfigViewportsNoTaskBarIcon) ImGui::Text("io.ConfigViewportsNoTaskBarIcon");
        if (io.ConfigViewportsNoDecoration) ImGui::Text("io.ConfigViewportsNoDecoration");
        if (io.ConfigViewportsNoDefaultParent) ImGui::Text("io.ConfigViewportsNoDefaultParent");
        if (io.ConfigDockingNoSplit) ImGui::Text("io.ConfigDockingNoSplit");
        if (io.ConfigDockingWithShift) ImGui::Text("io.ConfigDockingWithShift");
        if (io.ConfigDockingAlwaysTabBar) ImGui::Text("io.ConfigDockingAlwaysTabBar");
        if (io.ConfigDockingTransparentPayload) ImGui::Text("io.ConfigDockingTransparentPayload");
        if (io.ConfigMacOSXBehaviors) ImGui::Text("io.ConfigMacOSXBehaviors");
        if (io.ConfigNavMoveSetMousePos) ImGui::Text("io.ConfigNavMoveSetMousePos");
        if (io.ConfigNavCaptureKeyboard) ImGui::Text("io.ConfigNavCaptureKeyboard");
        if (io.ConfigInputTextCursorBlink) ImGui::Text("io.ConfigInputTextCursorBlink");
        if (io.ConfigWindowsResizeFromEdges) ImGui::Text("io.ConfigWindowsResizeFromEdges");
        if (io.ConfigWindowsMoveFromTitleBarOnly) ImGui::Text("io.ConfigWindowsMoveFromTitleBarOnly");
        if (io.ConfigMemoryCompactTimer >= 0.0f)
            ImGui::Text("io.ConfigMemoryCompactTimer = %.1f", io.ConfigMemoryCompactTimer);
        ImGui::Text("io.BackendFlags: 0x%08X", io.BackendFlags);
        if (io.BackendFlags & ImGuiBackendFlags_HasGamepad) ImGui::Text(" HasGamepad");
        if (io.BackendFlags & ImGuiBackendFlags_HasMouseCursors) ImGui::Text(" HasMouseCursors");
        if (io.BackendFlags & ImGuiBackendFlags_HasSetMousePos) ImGui::Text(" HasSetMousePos");
        if (io.BackendFlags & ImGuiBackendFlags_PlatformHasViewports) ImGui::Text(" PlatformHasViewports");
        if (io.BackendFlags & ImGuiBackendFlags_HasMouseHoveredViewport) ImGui::Text(" HasMouseHoveredViewport");
        if (io.BackendFlags & ImGuiBackendFlags_RendererHasVtxOffset) ImGui::Text(" RendererHasVtxOffset");
        if (io.BackendFlags & ImGuiBackendFlags_RendererHasViewports) ImGui::Text(" RendererHasViewports");
        ImGui::Separator();
        ImGui::Text(
            "io.Fonts: %d fonts, Flags: 0x%08X, TexSize: %d,%d", io.Fonts->Fonts.Size, io.Fonts->Flags,
            io.Fonts->TexWidth, io.Fonts->TexHeight
        );
        ImGui::Text("io.DisplaySize: %.2f,%.2f", io.DisplaySize.x, io.DisplaySize.y);
        ImGui::Text(
            "io.DisplayFramebufferScale: %.2f,%.2f", io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y
        );
        ImGui::Separator();
        ImGui::Text("style.WindowPadding: %.2f,%.2f", style.WindowPadding.x, style.WindowPadding.y);
        ImGui::Text("style.WindowBorderSize: %.2f", style.WindowBorderSize);
        ImGui::Text("style.FramePadding: %.2f,%.2f", style.FramePadding.x, style.FramePadding.y);
        ImGui::Text("style.FrameRounding: %.2f", style.FrameRounding);
        ImGui::Text("style.FrameBorderSize: %.2f", style.FrameBorderSize);
        ImGui::Text("style.ItemSpacing: %.2f,%.2f", style.ItemSpacing.x, style.ItemSpacing.y);
        ImGui::Text("style.ItemInnerSpacing: %.2f,%.2f", style.ItemInnerSpacing.x, style.ItemInnerSpacing.y);

        if (copy_to_clipboard)
        {
            ImGui::LogText("\n```\n");
            ImGui::LogFinish();
        }
        ImGui::EndChild();
    }
    ImGui::End();
}

EngineEditorBase* EditorUIModule::CreateEditor(EditorType type)
{
    UID uid                            = GenerateUID();
    ResourceStateMachine* stateMachine = new ResourceStateMachine(uid, "State Machine " + std::to_string(uid));
    switch (type)
    {
    case EditorType::BASE:

        return new EngineEditorBase("Base Editor " + std::to_string(uid), uid);
        break;
    case EditorType::NODE:
        return new NodeEditor("NodeEditor_" + std::to_string(uid), uid);

    case EditorType::ANIMATION:
        return stateMachineEditor =
                   new StateMachineEditor("StateMachineEditor_" + std::to_string(uid), uid, stateMachine);

    case EditorType::TEXTURE:
        return new TextureEditor("TextureEditor_" + std::to_string(uid), uid);
        break;
    default:
        return nullptr;
    }
}

void EditorUIModule::UpdateGizmoDragState()
{
    if (ImGuizmo::IsUsingAny()) guizmoDragState = GizmoDragState::DRAGGING;
    else if (guizmoDragState == GizmoDragState::DRAGGING) guizmoDragState = GizmoDragState::RELEASED;
    else guizmoDragState = GizmoDragState::IDLE;
}

void EditorUIModule::EditorSettings(bool& editorSettingsMenu)
{
    if (!ImGui::Begin("Editor settings", &editorSettingsMenu))
    {
        ImGui::End();
        return;
    }

    if (ImGui::CollapsingHeader("Application"))
    {
        ImGui::SeparatorText("Information");

        std::string appName = ENGINE_NAME;
        char* charAppName   = &appName[0];
        ImGui::InputText("App Name", charAppName, strlen(charAppName), ImGuiInputTextFlags_ReadOnly);

        std::string organizationName = ORGANIZATION_NAME;
        char* charOrganizationName   = &organizationName[0];
        ImGui::InputText("Organization", charOrganizationName, strlen(ORGANIZATION_NAME), ImGuiInputTextFlags_ReadOnly);

        ImGui::SeparatorText("Ms and Fps Graph");
        FramePlots(vsync);
    }

    ImGui::Spacing();

    ImGui::SeparatorText("Modules Configuration");
    if (ImGui::CollapsingHeader("Window"))
    {
        WindowConfig(vsync);
    }

    ImGui::Spacing();
    if (ImGui::CollapsingHeader("Input"))
    {
        InputModule* inputModule = App->GetInputModule();
        const KeyState* keyboard = inputModule->GetKeyboard();
        ImGui::Text("Key Q: %s", (keyboard[SDL_SCANCODE_Q] ? "Pressed" : "Not Pressed"));
        ImGui::Text("Key E: %s", (keyboard[SDL_SCANCODE_E] ? "Pressed" : "Not Pressed"));
        ImGui::Text("Key W: %s", (keyboard[SDL_SCANCODE_W] ? "Pressed" : "Not Pressed"));
        ImGui::Text("Key A: %s", (keyboard[SDL_SCANCODE_A] ? "Pressed" : "Not Pressed"));
        ImGui::Text("Key S: %s", (keyboard[SDL_SCANCODE_S] ? "Pressed" : "Not Pressed"));
        ImGui::Text("Key D: %s", (keyboard[SDL_SCANCODE_D] ? "Pressed" : "Not Pressed"));
        ImGui::Text("Key F: %s", (keyboard[SDL_SCANCODE_F] ? "Pressed" : "Not Pressed"));
        ImGui::Text("Key G: %s", (keyboard[SDL_SCANCODE_G] ? "Pressed" : "Not Pressed"));
        ImGui::Text("Key H: %s", (keyboard[SDL_SCANCODE_H] ? "Pressed" : "Not Pressed"));
        ImGui::Text("Key J: %s", (keyboard[SDL_SCANCODE_J] ? "Pressed" : "Not Pressed"));
        ImGui::Text("Key R: %s", (keyboard[SDL_SCANCODE_R] ? "Pressed" : "Not Pressed"));
        ImGui::Text("Key T: %s", (keyboard[SDL_SCANCODE_T] ? "Pressed" : "Not Pressed"));
        ImGui::Text("Key O: %s", (keyboard[SDL_SCANCODE_O] ? "Pressed" : "Not Pressed"));
        ImGui::Text("Key LSHIFT: %s", (keyboard[SDL_SCANCODE_LSHIFT] ? "Pressed" : "Not Pressed"));
        ImGui::Text("Key LALT: %s", (keyboard[SDL_SCANCODE_LALT] ? "Pressed" : "Not Pressed"));

        const KeyState* mouseButtons = inputModule->GetMouseButtons();
        ImGui::Text("Mouse Left: %s", (mouseButtons[SDL_BUTTON_LEFT - 1] ? "Pressed" : "Not Pressed"));
        ImGui::Text("Mouse Right: %s", (mouseButtons[SDL_BUTTON_RIGHT - 1] ? "Pressed" : "Not Pressed"));
        ImGui::Text("Mouse Middle: %s", (mouseButtons[SDL_BUTTON_MIDDLE - 1] ? "Pressed" : "Not Pressed"));

        const float2& mousePos = inputModule->GetMousePosition();
        ImGui::Text("Mouse Position: (%.f, %.f)", mousePos.x, mousePos.y);

        const float2& mouseMotion = inputModule->GetMouseMotion();
        ImGui::Text("Mouse Motion: (%.f, %.f)", mouseMotion[0], mouseMotion[1]);

        int mouseWheel = inputModule->GetMouseWheel();
        ImGui::Text("Mouse Wheel: %d", mouseWheel);
    }

    ImGui::Spacing();
    if (ImGui::CollapsingHeader("Editor camera"))
    {
        // TODO: ADD CAMERA MODULE AS TEMPORAL MEANWHILO THERE ARE NO GAMEOBJECTS
        // CameraConfig();
    }

    ImGui::Spacing();
    if (ImGui::CollapsingHeader("OpenGL"))
    {
        OpenGLConfig();
    }

    ImGui::Spacing();
    if (ImGui::CollapsingHeader("Hardware"))
    {
        HardwareConfig();
    }

    ImGui::Spacing();
    if (ImGui::CollapsingHeader("Physics"))
    {
        PhysicsConfig();
    }

    ImGui::End();
}

void EditorUIModule::FramePlots(bool& vsync)
{
    const int refreshRate = App->GetWindowModule()->GetDesktopDisplayMode().refresh_rate;

    if (vsync) maxYAxis = refreshRate * 1.66f;
    else maxYAxis = 1000.f * 1.66f;

    char title[25];
    const std::vector<float> frametimeVector(frametime.begin(), frametime.end());
    sprintf_s(title, 25, "Milliseconds %0.1f", frametime.back());
    ImGui::PlotHistogram(
        "##milliseconds", &frametimeVector[0], (int)frametimeVector.size(), 0, title, 0.0f, 40.0f, ImVec2(310, 100)
    );

    const std::vector<float> framerateVector(framerate.begin(), framerate.end());
    sprintf_s(title, 25, "Framerate %.1f", framerate.back());
    ImGui::PlotHistogram(
        "##framerate", &framerateVector.front(), (int)framerateVector.size(), 0, title, 0.0f, maxYAxis, ImVec2(310, 100)
    );
}

void EditorUIModule::WindowConfig(bool& vsync)
{
    float brightness = App->GetWindowModule()->GetBrightness();
    if (ImGui::SliderFloat("Brightness", &brightness, 0, 1)) App->GetWindowModule()->SetBrightness(brightness);

    SDL_DisplayMode& displayMode = App->GetWindowModule()->GetDesktopDisplayMode();
    int maxWidth                 = displayMode.w;
    int maxHeight                = displayMode.h;

    int width                    = App->GetWindowModule()->GetWidth();
    if (ImGui::SliderInt("Width", &width, 0, maxWidth)) App->GetWindowModule()->SetWidth(width);

    int height = App->GetWindowModule()->GetHeight();
    if (ImGui::SliderInt("Height", &height, 0, maxHeight)) App->GetWindowModule()->SetHeight(height);

    if (ImGui::Checkbox("Fullscreen", &fullscreen)) App->GetWindowModule()->SetFullscreen(fullscreen);

    ImGui::SameLine();

    if (ImGui::Checkbox("Full Desktop", &full_desktop)) App->GetWindowModule()->SetFullDesktop(full_desktop);

    if (ImGui::Checkbox("Borderless", &borderless)) App->GetWindowModule()->SetBorderless(borderless);

    ImGui::SameLine();

    // Set Resizable
    if (ImGui::Checkbox("Resizable", &resizable)) App->GetWindowModule()->SetResizable(resizable);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Restart to apply");

    // Set Vsync
    if (ImGui::Checkbox("Vsync", &vsync)) App->GetWindowModule()->SetVsync(vsync);
}

void EditorUIModule::CameraConfig() const
{
}

void EditorUIModule::OpenGLConfig()
{
    OpenGLModule* openGLModule = App->GetOpenGLModule();

    float clearRed             = openGLModule->GetClearRed();
    float clearGreen           = openGLModule->GetClearGreen();
    float clearBlue            = openGLModule->GetClearBlue();

    if (ImGui::SliderFloat("Red Clear Color", &clearRed, 0.f, 1.f)) openGLModule->SetClearRed(clearRed);
    if (ImGui::SliderFloat("Green Clear Color", &clearGreen, 0.f, 1.f)) openGLModule->SetClearGreen(clearGreen);
    if (ImGui::SliderFloat("Blue Clear Color", &clearBlue, 0.f, 1.f)) openGLModule->SetClearBlue(clearBlue);

    if (ImGui::Button("Reset Colors"))
    {
        clearRed   = DEFAULT_GL_CLEAR_COLOR_RED;
        clearGreen = DEFAULT_GL_CLEAR_COLOR_GREEN;
        clearBlue  = DEFAULT_GL_CLEAR_COLOR_BLUE;

        openGLModule->SetClearRed(clearRed);
        openGLModule->SetClearGreen(clearGreen);
        openGLModule->SetClearBlue(clearBlue);
    }

    ImGui::Separator();

    if (ImGui::Checkbox("Depth test", &depthTest))
    {
        openGLModule->SetDepthTest(depthTest);
    }

    if (ImGui::Checkbox("Face cull", &faceCulling))
    {
        openGLModule->SetFaceCull(faceCulling);
    }

    ImGui::Text("Front face mode");
    if (ImGui::RadioButton("Counter clock-wise", &frontFaceMode, GL_CCW))
    {
        openGLModule->SetFrontFaceMode(frontFaceMode);
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Clock-wise", &frontFaceMode, GL_CW))
    {
        openGLModule->SetFrontFaceMode(frontFaceMode);
    }

    ImGui::SeparatorText("Render Information");

    ImGui::Text("Draw calls:");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%d", App->GetOpenGLModule()->GetDrawCallsCount());

    const float currentTime = App->GetEngineTimer()->GetTime() / 1000.f;

    if (currentTime - lastTimeOpenGL > 1.f)
    {
        const unsigned int tps = static_cast<unsigned int>(App->GetOpenGLModule()->GetTrianglesPerSecond());
        lastTimeOpenGL         = currentTime;
        tpsStr                 = FormatWithCommas(tps);
    }

    ImGui::Text("Triangles per second:");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%s", tpsStr.c_str());

    ImGui::Text("Vertices:");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%d", App->GetOpenGLModule()->GetVerticesCount());
}

void EditorUIModule::GameTimerConfig() const
{
    GameTimer* gameTimer = App->GetGameTimer();
    float timeScale      = gameTimer->GetTimeScale();

    if (ImGui::Button("Play"))
    {
        gameTimer->Start();
    }
    ImGui::SameLine();
    if (ImGui::Button("Pause"))
    {
        gameTimer->TogglePause();
    }
    ImGui::SameLine();
    if (ImGui::Button("Step"))
    {
        gameTimer->Step();
    }
    ImGui::SameLine();
    if (ImGui::Button("Stop"))
    {
        gameTimer->Reset();
    }

    if (ImGui::SliderFloat("Time scale", &timeScale, 0, 4)) gameTimer->SetTimeScale(timeScale);

    ImGui::Separator();

    ImGui::Text("Frame count: %d", gameTimer->GetFrameCount());
    ImGui::Text("Game time: %.3f", gameTimer->GetTime() / 1000.0f);
    ImGui::Text("Delta time: %.3f", gameTimer->GetDeltaTime() / 1000.0f);
    ImGui::Text("Unscaled game time: %.3f", gameTimer->GetUnscaledTime() / 1000.0f);
    ImGui::Text("Unscaled delta time: %.3f", gameTimer->GetUnscaledDeltaTime() / 1000.0f);
    ImGui::Text("Reference time: %.3f", gameTimer->GetReferenceTime() / 1000.0f);
}

void EditorUIModule::HardwareConfig() const
{
    SDL_version sdlVersion;
    SDL_GetVersion(&sdlVersion);
    ImGui::Text("SDL Version:");
    ImGui::SameLine();
    ImGui::TextColored(
        ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%d.%d.%d", sdlVersion.major, sdlVersion.minor, sdlVersion.patch
    );

    const char* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    ImGui::Text("OpenGL Version:");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%s", version);

    ImGui::Separator();

    ImGui::Text("CPUs:");
    ImGui::SameLine();
    ImGui::TextColored(
        ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%d (Cache: %dkb)", SDL_GetCPUCount(), SDL_GetCPUCacheLineSize()
    );

    ImGui::Text("System RAM:");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%.2f GB", SDL_GetSystemRAM() / 1024.0f);

    ImGui::Text("Caps:");
    ShowCaps();

    ImGui::Separator();

    const char* vendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
    ImGui::Text("GPU:");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%s", vendor);

    const char* renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    ImGui::Text("Brand:");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%s", renderer);

    GLint vramBudget, vramUsage, vramAvailable, vramReserved;
    if (glewIsSupported("GL_NVX_gpu_memory_info"))
    {
        glGetIntegerv(GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX, &vramBudget);
        glGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &vramAvailable);
        glGetIntegerv(GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX, &vramReserved);
        vramUsage = vramBudget - vramAvailable;

        ImGui::Text("VRAM Budget:");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%.2f MB", vramBudget / 1024.0f);

        ImGui::Text("VRAM Usage:");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%.2f MB", vramUsage / 1024.0f);

        ImGui::Text("VRAM Available:");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%.2f MB", vramAvailable / 1024.0f);

        ImGui::Text("VRAM Reserved:");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%.2f MB", vramReserved / 1024.0f);
    }
}

void EditorUIModule::PhysicsConfig() const
{
    PhysicsModule* physicsModule = App->GetPhysicsModule();

    float worldGravity           = physicsModule->GetGravity();
    if (ImGui::InputFloat("World gravity", &worldGravity))
    {
        physicsModule->SetGravity(worldGravity);
    }

    std::vector<LayerBitset>& configBitset = physicsModule->GetLayerConfig();
    const char* tableStrings[IM_ARRAYSIZE(ColliderLayerStrings) + 1];
    tableStrings[0] = "LAYERS";

    for (int i = 1; i < IM_ARRAYSIZE(ColliderLayerStrings) + 1; ++i)
    {
        tableStrings[i] = ColliderLayerStrings[i - 1];
    }

    const int columns_count = IM_ARRAYSIZE(tableStrings);
    const int rows_count    = columns_count;

    static ImGuiTableFlags table_flags =
        ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY |
        ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_Hideable |
        ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_HighlightHoveredColumn;
    static ImGuiTableColumnFlags column_flags = ImGuiTableColumnFlags_AngledHeader | ImGuiTableColumnFlags_WidthFixed;

    if (ImGui::BeginTable(
            "table_angled_headers", columns_count, table_flags, ImVec2(0.0f, ImGui::GetTextLineHeightWithSpacing() * 12)
        ))
    {
        ImGui::TableSetupColumn(tableStrings[0], ImGuiTableColumnFlags_NoHide | ImGuiTableColumnFlags_NoReorder);
        for (int n = 1; n < columns_count; n++)
            ImGui::TableSetupColumn(tableStrings[n], column_flags);

        ImGui::TableAngledHeadersRow();
        ImGui::TableHeadersRow(); // Draw remaining headers and allow access to context-menu and other functions.
        for (int row = 1; row < rows_count; row++)
        {
            ImGui::PushID(row);
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::Text(tableStrings[row]);
            for (int column = 1; column < columns_count; column++)
            {
                if (ImGui::TableSetColumnIndex(column) && row > 0)
                {
                    bool currentBitValue = configBitset[row - 1][column - 1];
                    ImGui::PushID(column);
                    if (ImGui::Checkbox("", &currentBitValue))
                    {
                        configBitset[row - 1][column - 1].flip();
                        physicsModule->RebuildWorld();
                    }
                    ImGui::PopID();
                }
            }
            ImGui::PopID();
        }
        ImGui::EndTable();
    }

    // ImGui::ShowDemoWindow();
}

void EditorUIModule::ShowCaps() const
{
    struct CPUFeature
    {
        SDL_bool (*check)();
        const char* name;
    };

    const CPUFeature features[] = {
        {SDL_HasRDTSC,   "RDTSC"  },
        {SDL_HasAltiVec, "AltiVec"},
        {SDL_HasMMX,     "MMX"    },
        {SDL_Has3DNow,   "3DNow"  },
        {SDL_HasSSE,     "SSE"    },
        {SDL_HasSSE2,    "SSE2"   },
        {SDL_HasSSE3,    "SSE3"   },
        {SDL_HasSSE41,   "SSE41"  },
        {SDL_HasSSE42,   "SSE42"  },
        {SDL_HasAVX,     "AVX"    },
        {SDL_HasAVX2,    "AVX2"   },
        {SDL_HasAVX512F, "AVX512F"},
        {SDL_HasARMSIMD, "ARMSIMD"},
        {SDL_HasNEON,    "NEON"   }
    };

    int cont = 0;
    for (const auto& feature : features)
    {
        if (feature.check() == SDL_TRUE)
        {
            if (cont != 4) ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%s", feature.name);
            cont++;
        }
    }
}

std::string EditorUIModule::FormatWithCommas(unsigned int number) const
{
    std::stringstream ss;
    ss.imbue(std::locale("en_US.UTF-8")); // use commas
    ss << number;
    return ss.str();
}

void EditorUIModule::RequestExit()
{
    closeApplication = true;
}

void EditorUIModule::ToggleFullscreen()
{
    const bool current = App->GetWindowModule()->GetFullscreen();
    App->GetWindowModule()->SetFullscreen(!current);
}

void EditorUIModule::ToggleVSync()
{
    const bool current = App->GetWindowModule()->GetVsync();
    App->GetWindowModule()->SetVsync(!current);
}
