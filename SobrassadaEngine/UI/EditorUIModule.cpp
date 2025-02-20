#include "EditorUIModule.h"

#include "Application.h"
#include "FileSystem.h"
#include "GameTimer.h"
#include "InputModule.h"
#include "LibraryModule.h"
#include "OpenGLModule.h"
#include "QuadtreeViewer.h"
#include "SceneImporter.h"
#include "SceneModule.h"
#include "WindowModule.h"

#include "Component.h"
#include "GameObject.h"
#include "Root/RootComponent.h"

#include "glew.h"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl2.h"
#include <string>

#include <cstring>
#include <filesystem>

#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_EXTERNAL_IMAGE
#define TINYGLTF_IMPLEMENTATION /* Only in one of the includes */
#include <tiny_gltf.h>          // TODO Remove

#include "InputModule.h"

EditorUIModule::EditorUIModule()
    : width(0), height(0), closeApplication(false), consoleMenu(false), import(false), load(false), save(false),
      editorSettingsMenu(false)
{
}

EditorUIModule::~EditorUIModule()
{
}

bool EditorUIModule::Init()
{
    ImGui::CreateContext();
    ImGuiIO& io     = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // IF using Docking Branch

    ImGui_ImplSDL2_InitForOpenGL(App->GetWindowModule()->window, App->GetOpenGLModule()->GetContext());
    ImGui_ImplOpenGL3_Init("#version 460");

    quadtreeViewer = new QuadtreeViewer();

    width          = App->GetWindowModule()->GetWidth();
    height         = App->GetWindowModule()->GetHeight();

    startPath      = std::filesystem::current_path().string();
    libraryPath    = startPath + DELIMITER + SCENES_PATH;

    App->GetInputModule()->SubscribeToEvent(SDL_SCANCODE_W, [&] { mCurrentGizmoOperation = ImGuizmo::TRANSLATE; });
    App->GetInputModule()->SubscribeToEvent(SDL_SCANCODE_E, [&] { mCurrentGizmoOperation = ImGuizmo::ROTATE; });
    App->GetInputModule()->SubscribeToEvent(SDL_SCANCODE_R, [&] { mCurrentGizmoOperation = ImGuizmo::SCALE; });

    return true;
}

update_status EditorUIModule::PreUpdate(float deltaTime)
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
    ImGuizmo::SetOrthographic(false); // TODO Implement orthographic camera
    ImGuizmo::BeginFrame();
    ImGui::DockSpaceOverViewport();

    return UPDATE_CONTINUE;
}

update_status EditorUIModule::Update(float deltaTime)
{
    LimitFPS(deltaTime);
    AddFramePlotData(deltaTime);
    return UPDATE_CONTINUE;
}

update_status EditorUIModule::RenderEditor(float deltaTime)
{
    Draw();

    if (quadtreeViewerViewport) quadtreeViewer->Render(quadtreeViewerViewport);

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    return UPDATE_CONTINUE;
}

update_status EditorUIModule::PostUpdate(float deltaTime)
{

    if (closeApplication) return UPDATE_STOP;

    return UPDATE_CONTINUE;
}

bool EditorUIModule::ShutDown()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    framerate.clear();
    frametime.clear();

    delete quadtreeViewer;

    return true;
}

void EditorUIModule::LimitFPS(float deltaTime) const
{
    if (deltaTime == 0) return;

    float targetFrameTime = 1000.f / maxFPS;

    if (maxFPS != 0)
    {
        if (deltaTime < targetFrameTime) SDL_Delay(static_cast<Uint32>(targetFrameTime - deltaTime));
    }
}

void EditorUIModule::AddFramePlotData(float deltaTime)
{
    if (deltaTime == 0) return;

    float newFrametime = deltaTime * 1000.f;
    float newFramerate = 1.f / deltaTime;

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

    if (import) ImportDialog(import);

    if (load) LoadDialog(load);

    if (save) SaveDialog(save);

    if (editorSettingsMenu) EditorSettings(editorSettingsMenu);
}

void EditorUIModule::MainMenu()
{
    ImGui::BeginMainMenuBar();

    // File menu
    if (ImGui::BeginMenu("File"))
    {
        if (ImGui::MenuItem("Create", "")) App->GetSceneModule()->CreateScene();

        if (ImGui::MenuItem("Import", "", import)) import = !import;

        if (ImGui::MenuItem("Load", "", load)) load = !load;

        if (ImGui::MenuItem("Save"))
        {
            if (!App->GetLibraryModule()->SaveScene(libraryPath.c_str(), SaveMode::Save))
            {
                save = !save;
            }
        }

        if (ImGui::MenuItem("Save as")) save = !save;

        if (ImGui::MenuItem("Quit")) closeApplication = true;

        ImGui::EndMenu();
    }

    // Windows menu
    if (ImGui::BeginMenu("Window"))
    {
        if (ImGui::MenuItem("Console", "", consoleMenu)) consoleMenu = !consoleMenu;

        if (ImGui::BeginMenu("Scene"))
        {
            if (ImGui::MenuItem("Hierarchy", "", hierarchyMenu)) hierarchyMenu = !hierarchyMenu;
            if (ImGui::MenuItem("Inspector", "", inspectorMenu)) inspectorMenu = !inspectorMenu;

            ImGui::EndMenu();
        }

        if (ImGui::MenuItem("Quadtree", "", quadtreeViewerViewport)) quadtreeViewerViewport = !quadtreeViewerViewport;

        if (ImGui::MenuItem("Editor settings", "", editorSettingsMenu)) editorSettingsMenu = !editorSettingsMenu;

        if (ImGui::MenuItem("About", "", aboutMenu)) aboutMenu = !aboutMenu;

        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Assets"))
    {
        if (ImGui::MenuItem("Cube Mesh"))
        {
            GameObject* selectedGameObject = App->GetSceneModule()->GetSeletedGameObject();
            if (selectedGameObject != nullptr)
            {
                GameObject* newGameObject = new GameObject(selectedGameObject->GetUID(), "Cube Mesh GameObject");

                selectedGameObject->AddChildren(newGameObject->GetUID());

                App->GetSceneModule()->AddGameObject(newGameObject->GetUID(), newGameObject);
                newGameObject->ComponentGlobalTransformUpdated();
                selectedGameObject->ComponentGlobalTransformUpdated();

                RootComponent* root = newGameObject->GetRootComponent();
                root->CreateComponent(COMPONENT_CUBE_MESH);
            }
        }
        ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
}

void EditorUIModule::LoadDialog(bool& load)
{
    ImGui::SetNextWindowSize(ImVec2(width * 0.25f, height * 0.4f), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin("Load Scene", &load, ImGuiWindowFlags_NoCollapse))
    {
        ImGui::End();
        return;
    }

    static std::string inputFile = "";
    static std::vector<std::string> files;

    ImGui::BeginChild("scrollFiles", ImVec2(0, -30), ImGuiChildFlags_Borders);

    if (FileSystem::Exists(libraryPath.c_str()))
    {
        // Only scenes for now
        if (ImGui::TreeNode("Scenes/"))
        {
            GetFilesSorted(libraryPath, files);

            static int selected = -1;

            for (int i = 0; i < files.size(); i++)
            {
                const std::string& file = files[i];
                if (ImGui::Selectable(file.c_str(), selected == i))
                {
                    selected  = i;
                    inputFile = file;
                }
            }

            ImGui::TreePop();
        }
    }

    ImGui::EndChild();

    ImGui::InputText("##filename", &inputFile[0], inputFile.size(), ImGuiInputTextFlags_ReadOnly);

    ImGui::SameLine();

    if (ImGui::Button("Ok", ImVec2(0, 0)))
    {
        if (!inputFile.empty())
        {
            std::string loadPath = SCENES_PATH + inputFile;
            App->GetLibraryModule()->LoadScene(loadPath.c_str());
        }
        inputFile = "";
        load      = false;
    }

    ImGui::SameLine();

    if (ImGui::Button("Cancel", ImVec2(0, 0)))
    {
        inputFile = "";
        load      = false;
    }

    ImGui::End();
}

void EditorUIModule::SaveDialog(bool& save)
{
    ImGui::SetNextWindowSize(ImVec2(width * 0.25f, height * 0.4f), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin("Save Scene", &save, ImGuiWindowFlags_NoCollapse))
    {
        ImGui::End();
        return;
    }

    static std::vector<std::string> files;
    static char inputFile[32];

    ImGui::BeginChild("scrollFiles", ImVec2(0, -30), ImGuiChildFlags_Borders);

    if (FileSystem::Exists(libraryPath.c_str()))
    {
        if (ImGui::TreeNode("Scenes/"))
        {
            GetFilesSorted(libraryPath, files);

            for (int i = 0; i < files.size(); i++)
            {
                const std::string& file = files[i];
                if (ImGui::Selectable(file.c_str()))
                {
                }
            }

            ImGui::TreePop();
        }
    }

    ImGui::EndChild();

    ImGui::InputText("##filename", inputFile, IM_ARRAYSIZE(inputFile));

    ImGui::SameLine();

    if (ImGui::Button("Ok", ImVec2(0, 0)))
    {
        if (strlen(inputFile) > 0)
        {
            std::string savePath = libraryPath + inputFile + SCENE_EXTENSION;
            App->GetLibraryModule()->SaveScene(savePath.c_str(), SaveMode::SaveAs);
        }
        inputFile[0] = '\0';
        save         = false;
    }

    ImGui::SameLine();

    if (ImGui::Button("Cancel", ImVec2(0, 0)))
    {
        inputFile[0] = '\0';
        save         = false;
    }

    ImGui::End();
}

void EditorUIModule::ImportDialog(bool& import)
{
    ImGui::SetNextWindowSize(ImVec2(width * 0.4f, height * 0.4f), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin("Import Asset", &import, ImGuiWindowFlags_NoCollapse))
    {
        ImGui::End();
        return;
    }

    static std::string currentPath = startPath;
    static std::vector<std::string> accPaths;
    static bool loadButtons = true;

    static std::vector<std::string> files;
    static bool loadFiles = false;
    static std::vector<std::string> filteredFiles;
    static bool loadFilteredFiles = false;

    static char searchQuery[32];
    static char lastQuery[32] = "default";
    static bool showDrives    = false;

    if (ImGui::Button("Drives"))
    {
        FileSystem::GetDrives(files);
        showDrives = true;
    }

    ImGui::SameLine();
    ImGui::Text("|");

    if (!showDrives)
    {
        ImGui::SameLine();

        if (loadButtons)
        {
            FileSystem::SplitAccumulatedPath(currentPath, accPaths);
            loadButtons = false;
        }

        for (size_t i = 0; i < accPaths.size(); i++)
        {
            const std::string& accPath = accPaths[i];

            std::string buttonLabel    = (i == 0) ? accPath : FileSystem::GetFileNameWithExtension(accPath);

            if (ImGui::Button(buttonLabel.c_str()))
            {
                currentPath    = accPath;
                showDrives     = false;
                loadFiles      = true;
                loadButtons    = true;
                searchQuery[0] = '\0';
            }

            if (i < accPaths.size() - 1) ImGui::SameLine();
        }
    }

    ImGui::Separator();

    ImGui::Text("Search:");
    ImGui::SameLine();
    ImGui::InputText("##search", searchQuery, IM_ARRAYSIZE(searchQuery));

    static std::string inputFile = "";

    ImGui::BeginChild("scrollFiles", ImVec2(0, -70), ImGuiChildFlags_Borders);

    if (showDrives)
    {
        for (const std::string& drive : files)
        {
            if (ImGui::Selectable(drive.c_str()))
            {
                currentPath = drive;
                showDrives  = false;
                loadFiles   = true;
                loadButtons = true;
            }
        }
    }
    else
    {
        // root directory add delimiter
        if (currentPath.back() == ':') currentPath += DELIMITER;

        if (files.empty() || loadFiles)
        {
            GetFilesSorted(currentPath, files);

            files.insert(files.begin(), "..");

            loadFiles         = false;
            loadFilteredFiles = true;
        }

        if (strcmp(lastQuery, searchQuery) != 0 || loadFilteredFiles) // if the search query has changed
        {
            strcpy_s(lastQuery, searchQuery);

            filteredFiles.clear();

            for (const std::string& file : files)
            {
                if (file.find(searchQuery) != std::string::npos)
                {
                    filteredFiles.push_back(file);
                }
            }

            loadFilteredFiles = false;
        }

        static int selected = -1;

        for (int i = 0; i < filteredFiles.size(); i++)
        {
            const std::string& file = filteredFiles[i];
            std::string filePath    = currentPath + DELIMITER + file;
            bool isDirectory        = FileSystem::IsDirectory(filePath.c_str());

            std::string tag         = isDirectory ? "[Dir] " : "[File] ";
            std::string fileWithTag = tag + file;

            if (ImGui::Selectable(fileWithTag.c_str(), selected == i))
            {
                selected = i;

                if (file == "..")
                {
                    currentPath = FileSystem::GetParentPath(currentPath);
                    inputFile   = "";
                    selected    = -1;
                    GetFilesSorted(currentPath, files);
                    searchQuery[0] = '\0';
                    loadFiles      = true;
                    loadButtons    = true;
                }
                else if (isDirectory)
                {
                    currentPath = filePath;
                    inputFile   = "";
                    selected    = -1;
                    GetFilesSorted(currentPath, files);
                    searchQuery[0] = '\0';
                    loadFiles      = true;
                    loadButtons    = true;
                }
                else
                {
                    inputFile = FileSystem::GetFileNameWithExtension(file);
                }
            }
        }
    }

    ImGui::EndChild();

    ImGui::Dummy(ImVec2(0, 10));
    ImGui::Text("File Name:");
    ImGui::SameLine();
    ImGui::InputText("##filename", &inputFile[0], inputFile.size(), ImGuiInputTextFlags_ReadOnly);

    if (ImGui::Button("Cancel", ImVec2(0, 0)))
    {
        inputFile      = "";
        currentPath    = startPath;
        import         = false;
        showDrives     = false;
        searchQuery[0] = '\0';
        loadFiles      = true;
    }

    ImGui::SameLine();

    if (ImGui::Button("Ok", ImVec2(0, 0)))
    {
        if (!inputFile.empty())
        {
            std::string importPath = currentPath + DELIMITER + inputFile;
            SceneImporter::Import(importPath.c_str());
        }
        inputFile      = "";
        currentPath    = startPath;
        import         = false;
        showDrives     = false;
        searchQuery[0] = '\0';
        loadFiles      = true;
    }

    ImGui::End();
}

void EditorUIModule::GetFilesSorted(const std::string& currentPath, std::vector<std::string>& files)
{
    // files & dir in the current directory
    FileSystem::GetAllInDirectory(currentPath, files);

    std::sort(
        files.begin(), files.end(),
        [&](const std::string& a, const std::string& b)
        {
            bool isDirA = FileSystem::IsDirectory((currentPath + DELIMITER + a).c_str());
            bool isDirB = FileSystem::IsDirectory((currentPath + DELIMITER + b).c_str());

            if (isDirA != isDirB) return isDirA > isDirB;
            return a < b;
        }
    );
}

void EditorUIModule::Console(bool& consoleMenu) const
{
    if (!ImGui::Begin("Console", &consoleMenu))
    {
        ImGui::End();
        return;
    }

    for (const char* log : *Logs)
    {
        ImGui::TextUnformatted(log);
    }

    // Autoscroll only if the scroll is in the bottom position
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
    {
        ImGui::SetScrollHereY(1.0f);
    }

    ImGui::End();
}

bool EditorUIModule::RenderTransformWidget(
    Transform& localTransform, Transform& globalTransform, const Transform& parentTransform
)
{
    bool positionValueChanged = false;
    bool rotationValueChanged = false;
    bool scaleValueChanged    = false;
    static bool lockScaleAxis = false;
    static int transformType  = LOCAL;
    static int pivotType      = OBJECT;
    float3 originalScale;

    ImGui::SeparatorText("Transform");
    ImGui::RadioButton("Use object pivot", &pivotType, OBJECT);
    // TODO Add later if necessary
    // ImGui::SameLine();
    // ImGui::RadioButton("Use root pivot", &pivotType, ROOT);

    ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
    if (ImGui::BeginTabBar("TransformType##", tab_bar_flags))
    {
        if (ImGui::BeginTabItem("Local transform"))
        {
            transformType = LOCAL;
            originalScale = float3(localTransform.scale);
            RenderBasicTransformModifiers(
                localTransform, lockScaleAxis, positionValueChanged, rotationValueChanged, scaleValueChanged
            );
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Global transform"))
        {
            transformType = GLOBAL;
            originalScale = float3(globalTransform.scale);
            RenderBasicTransformModifiers(
                globalTransform, lockScaleAxis, positionValueChanged, rotationValueChanged, scaleValueChanged
            );
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    Transform& outputTransform = transformType == LOCAL ? localTransform : globalTransform;

    if (positionValueChanged || rotationValueChanged || scaleValueChanged)
    {
        if (scaleValueChanged && lockScaleAxis)
        {
            float scaleFactor = 1;
            if (outputTransform.scale.x != originalScale.x)
            {
                scaleFactor = originalScale.x == 0 ? 1 : outputTransform.scale.x / originalScale.x;
            }
            else if (outputTransform.scale.y != originalScale.y)
            {
                scaleFactor = originalScale.y == 0 ? 1 : outputTransform.scale.y / originalScale.y;
            }
            else if (outputTransform.scale.z != originalScale.z)
            {
                scaleFactor = originalScale.z == 0 ? 1 : outputTransform.scale.z / originalScale.z;
            }
            originalScale         *= scaleFactor;
            outputTransform.scale  = originalScale;
        }

        if (transformType == GLOBAL)
        {
            localTransform.Set(globalTransform - parentTransform);
        }
    }

    return positionValueChanged || rotationValueChanged || scaleValueChanged;
}

bool EditorUIModule::RenderImGuizmo(Transform& gameObjectTransform)
{
    float4x4 view = float4x4(App->GetCameraModule()->GetViewMatrix());
    view.Transpose();

    float4x4 proj = float4x4(App->GetCameraModule()->GetProjectionMatrix());
    proj.Transpose();

    float4x4 gizmoMatrix          = float4x4::identity;
    gameObjectTransform.rotation *= RAD_DEGREE_CONV;
    ImGuizmo::RecomposeMatrixFromComponents(
        gameObjectTransform.position.ptr(), gameObjectTransform.rotation.ptr(), gameObjectTransform.scale.ptr(),
        gizmoMatrix.ptr()
    );

    Manipulate(view.ptr(), proj.ptr(), mCurrentGizmoOperation, ImGuizmo::MODE::LOCAL, gizmoMatrix.ptr());

    ImGuizmo::DecomposeMatrixToComponents(
        gizmoMatrix.ptr(), gameObjectTransform.position.ptr(), gameObjectTransform.rotation.ptr(),
        gameObjectTransform.scale.ptr()
    );
    gameObjectTransform.rotation /= RAD_DEGREE_CONV;

    return ImGuizmo::IsUsing();
}

void EditorUIModule::RenderBasicTransformModifiers(
    Transform& outputTransform, bool& lockScaleAxis, bool& positionValueChanged, bool& rotationValueChanged,
    bool& scaleValueChanged
)
{
    static bool bUseRad   = true;

    positionValueChanged |= ImGui::InputFloat3("Position", &outputTransform.position[0]);

    if (!bUseRad)
    {
        outputTransform.rotation *= RAD_DEGREE_CONV;
    }
    rotationValueChanged |= ImGui::InputFloat3("Rotation", &outputTransform.rotation[0]);
    if (!bUseRad)
    {
        outputTransform.rotation /= RAD_DEGREE_CONV;
    }
    ImGui::SameLine();
    ImGui::Checkbox("Radians", &bUseRad);
    scaleValueChanged |= ImGui::InputFloat3("Scale", &outputTransform.scale[0]);
    ImGui::SameLine();
    ImGui::Checkbox("Lock axis", &lockScaleAxis);
}

UID EditorUIModule::RenderResourceSelectDialog(
    const char* id, const std::unordered_map<std::string, UID>& availableResources
)
{
    UID result = CONSTANT_EMPTY_UID;
    if (ImGui::BeginPopup(id))
    {
        static char searchText[255] = "";
        ImGui::InputText("Search", searchText, 255);

        ImGui::Separator();
        if (ImGui::BeginListBox("##ComponentList", ImVec2(-FLT_MIN, 5 * ImGui::GetTextLineHeightWithSpacing())))
        {
            for (const auto& valuePair : availableResources)
            {
                {
                    if (valuePair.first.find(searchText) != std::string::npos)
                    {
                        if (ImGui::Selectable(valuePair.first.c_str(), false))
                        {
                            result = valuePair.second;
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

void EditorUIModule::About(bool& aboutMenu) const
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
    ImGui::Text("%s is licensed under the MIT License, see LICENSE for more information.", ENGINE_NAME);

    static bool show_config_info = false;
    ImGui::Checkbox("Config/Build Information", &show_config_info);
    if (show_config_info)
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

void EditorUIModule::EditorSettings(bool& editorSettingsMenu)
{
    if (!ImGui::Begin("Editor settings", &editorSettingsMenu))
    {
        ImGui::End();
        return;
    }

    static bool vsync = VSYNC;

    if (ImGui::CollapsingHeader("Application"))
    {
        ImGui::SeparatorText("Information");
        ImGui::InputText(
            "App Name", const_cast<char*>(ENGINE_NAME), IM_ARRAYSIZE(ENGINE_NAME), ImGuiInputTextFlags_ReadOnly
        );
        ImGui::InputText(
            "Organization", const_cast<char*>(ORGANIZATION_NAME), IM_ARRAYSIZE(ORGANIZATION_NAME),
            ImGuiInputTextFlags_ReadOnly
        );

        ImGui::SeparatorText("Ms and Fps Graph");
        FramePlots(vsync);
    }

    ImGui::Spacing();
    if (ImGui::CollapsingHeader("Game timer"))
    {
        GameTimerConfig();
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

    ImGui::End();
}

void EditorUIModule::FramePlots(bool& vsync)
{
    static int refreshRate = App->GetWindowModule()->GetDesktopDisplayMode().refresh_rate;

    static int sliderFPS   = 0;
    ImGui::SliderInt("Max FPS", &sliderFPS, 0, refreshRate);

    static float maxYAxis;
    if (sliderFPS == 0)
    {
        if (vsync)
        {
            maxFPS   = refreshRate;
            maxYAxis = maxFPS * 1.66f;
        }
        else
        {
            maxFPS   = 0;
            maxYAxis = 1000.f * 1.66f;
        }
    }
    else
    {
        maxFPS   = sliderFPS;
        maxYAxis = maxFPS * 1.66f;
    }

    ImGui::Text("Limit Framerate: ");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%d", maxFPS);

    char title[25];
    std::vector<float> frametimeVector(frametime.begin(), frametime.end());
    sprintf_s(title, 25, "Milliseconds %0.1f", frametime.back());
    ImGui::PlotHistogram(
        "##milliseconds", &frametimeVector[0], (int)frametimeVector.size(), 0, title, 0.0f, 40.0f, ImVec2(310, 100)
    );

    std::vector<float> framerateVector(framerate.begin(), framerate.end());
    sprintf_s(title, 25, "Framerate %.1f", framerate.back());
    ImGui::PlotHistogram(
        "##framerate", &framerateVector.front(), (int)framerateVector.size(), 0, title, 0.0f, maxYAxis, ImVec2(310, 100)
    );
}

void EditorUIModule::WindowConfig(bool& vsync) const
{
    static bool borderless   = BORDERLESS;
    static bool full_desktop = FULL_DESKTOP;
    static bool resizable    = RESIZABLE;
    static bool fullscreen   = FULLSCREEN;

    // Brightness Slider
    float brightness         = App->GetWindowModule()->GetBrightness();
    if (ImGui::SliderFloat("Brightness", &brightness, 0, 1)) App->GetWindowModule()->SetBrightness(brightness);

    SDL_DisplayMode& displayMode = App->GetWindowModule()->GetDesktopDisplayMode();
    int maxWidth                 = displayMode.w;
    int maxHeight                = displayMode.h;

    // Width Slider
    int width                    = App->GetWindowModule()->GetWidth();
    if (ImGui::SliderInt("Width", &width, 0, maxWidth)) App->GetWindowModule()->SetWidth(width);

    // Height Slider
    int height = App->GetWindowModule()->GetHeight();
    if (ImGui::SliderInt("Height", &height, 0, maxHeight)) App->GetWindowModule()->SetHeight(height);

    // Set Fullscreen
    if (ImGui::Checkbox("Fullscreen", &fullscreen)) App->GetWindowModule()->SetFullscreen(fullscreen);
    ImGui::SameLine();

    // Set Resizable
    if (ImGui::Checkbox("Resizable", &resizable)) App->GetWindowModule()->SetResizable(resizable);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Restart to apply");

    // Set Borderless
    if (ImGui::Checkbox("Borderless", &borderless)) App->GetWindowModule()->SetBorderless(borderless);
    ImGui::SameLine();

    // Set Full Desktop
    if (ImGui::Checkbox("Full Desktop", &full_desktop)) App->GetWindowModule()->SetFullDesktop(full_desktop);

    // Set Vsync
    if (ImGui::Checkbox("Vsync", &vsync)) App->GetWindowModule()->SetVsync(vsync);
}

void EditorUIModule::CameraConfig() const
{
}

void EditorUIModule::OpenGLConfig() const
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

    static bool depthTest = true;

    if (ImGui::Checkbox("Depth test", &depthTest))
    {
        openGLModule->SetDepthTest(depthTest);
    }

    static bool faceCulling = true;
    if (ImGui::Checkbox("Face cull", &faceCulling))
    {
        openGLModule->SetFaceCull(faceCulling);
    }

    static int frontFaceMode = GL_CCW;
    bool changed             = false;
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

void EditorUIModule::ShowCaps() const
{
    static const CPUFeature features[] = {
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
