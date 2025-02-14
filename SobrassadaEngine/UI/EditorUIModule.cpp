#include "EditorUIModule.h"

#include "Application.h"
#include "LibraryModule.h"
#include "EditorViewport.h"
#include "FileSystem.h"
#include "OpenGLModule.h"
#include "SceneImporter.h"
#include "WindowModule.h"
#include "SceneModule.h"

#include "Component.h"

#include "glew.h"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl2.h"
#include <cstring>
#include <filesystem>

#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_EXTERNAL_IMAGE
#include <tiny_gltf.h>

EditorUIModule::EditorUIModule()
    : width(0), height(0), closeApplication(false), consoleMenu(false), import(false), load(false), save(false),
      editorSettingsMenu(false)
{
}

EditorUIModule::~EditorUIModule() {}

bool EditorUIModule::Init()
{
    ImGui::CreateContext();
    ImGuiIO &io     = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // IF using Docking Branch

    ImGui_ImplSDL2_InitForOpenGL(App->GetWindowModule()->window, App->GetOpenGLModule()->GetContext());
    ImGui_ImplOpenGL3_Init("#version 460");

    editorViewport = new EditorViewport();

    width          = App->GetWindowModule()->GetWidth();
    height         = App->GetWindowModule()->GetHeight();

    startPath      = std::filesystem::current_path().string();
    libraryPath    = startPath + DELIMITER + SCENES_PATH;

    return true;
}

update_status EditorUIModule::PreUpdate(float deltaTime)
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
    ImGui::DockSpaceOverViewport();

    return UPDATE_CONTINUE;
}

update_status EditorUIModule::Update(float deltaTime)
{
    AddFramePlotData(deltaTime);
    return UPDATE_CONTINUE;
}

update_status EditorUIModule::RenderEditor(float deltaTime)
{
    Draw();

    editorViewport->Render();
    
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

    delete editorViewport;

    return true;
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

    if (import) ImportDialog(import);

    if (load) LoadDialog(load);

    if (save) SaveDialog(save);

    if (editorSettingsMenu) EditorSettings(editorSettingsMenu);
}

void EditorUIModule::MainMenu()
{
    ImGui::BeginMainMenuBar();

    if (ImGui::BeginMenu("File"))
    {
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

    // General menu
    if (ImGui::BeginMenu("View"))
    {
        if (ImGui::MenuItem("Console", "", consoleMenu)) consoleMenu = !consoleMenu;

        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Window"))
    {
        if (ImGui::BeginMenu("General"))
        {
            if (ImGui::MenuItem("Hierarchy")) hierarchyMenu = !hierarchyMenu;
            if (ImGui::MenuItem("Inspector")) inspectorMenu = !inspectorMenu;
            
            ImGui::EndMenu();
        }

        ImGui::EndMenu();
    }


    // Settings menu
    if (ImGui::BeginMenu("Settings"))
    {
        if (ImGui::MenuItem("Editor settings", "", editorSettingsMenu)) editorSettingsMenu = !editorSettingsMenu;

        ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
}

void EditorUIModule::LoadDialog(bool &load)
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
                const std::string &file = files[i];
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

void EditorUIModule::SaveDialog(bool &save)
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
                const std::string &file = files[i];
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

void EditorUIModule::ImportDialog(bool &import)
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
            const std::string &accPath = accPaths[i];

            std::string buttonLabel    = (i == 0) ? accPath : FileSystem::GetFileNameWithExtension(accPath);

            if (ImGui::Button(buttonLabel.c_str()))
            {
                currentPath = accPath;
                showDrives  = false;
                loadFiles   = true;
                loadButtons = true;
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
        for (const std::string &drive : files)
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

            for (const std::string &file : files)
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
            const std::string &file = filteredFiles[i];
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

void EditorUIModule::GetFilesSorted(const std::string &currentPath, std::vector<std::string> &files)
{
    // files & dir in the current directory
    FileSystem::GetAllInDirectory(currentPath, files);

    std::sort(
        files.begin(), files.end(),
        [&](const std::string &a, const std::string &b)
        {
            bool isDirA = FileSystem::IsDirectory((currentPath + DELIMITER + a).c_str());
            bool isDirB = FileSystem::IsDirectory((currentPath + DELIMITER + b).c_str());

            if (isDirA != isDirB) return isDirA > isDirB;
            return a < b;
        }
    );
}

void EditorUIModule::Console(bool &consoleMenu)
{
    if (!ImGui::Begin("Console", &consoleMenu))
    {
        ImGui::End();
        return;
    }

    for (const char *log : *Logs)
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

bool EditorUIModule::RenderTransformWidget(Transform &localTransform, Transform &globalTransform, const Transform& parentTransform)
{
    bool positionValueChanged = false;
    bool rotationValueChanged = false;
    bool scaleValueChanged = false;
    static bool lockScaleAxis = false;
    static int transformType = LOCAL;
    static int pivotType = OBJECT;
    float3 originalScale;
    
    ImGui::SeparatorText("Transform");
    ImGui::RadioButton("Use object pivot", &pivotType, OBJECT);
    // TODO Add later if necessary
    //ImGui::SameLine();
    //ImGui::RadioButton("Use root pivot", &pivotType, ROOT);
    
    ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
    if (ImGui::BeginTabBar("TransformType##", tab_bar_flags))
    {
        if (ImGui::BeginTabItem("Local transform"))
        {
            transformType = LOCAL;
            originalScale = float3(localTransform.scale);
            RenderBasicTransformModifiers(localTransform, lockScaleAxis, positionValueChanged, rotationValueChanged, scaleValueChanged);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Global transform"))
        {
            transformType = GLOBAL;
            originalScale = float3(globalTransform.scale);
            RenderBasicTransformModifiers(globalTransform, lockScaleAxis, positionValueChanged, rotationValueChanged, scaleValueChanged);
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
                
            } else if (outputTransform.scale.y != originalScale.y)
            {
                scaleFactor = originalScale.y == 0 ? 1 : outputTransform.scale.y / originalScale.y;
            } else if (outputTransform.scale.z != originalScale.z)
            {
                scaleFactor = originalScale.z == 0 ? 1 : outputTransform.scale.z / originalScale.z;
            }
            originalScale *= scaleFactor;
            outputTransform.scale = originalScale;
        }
        
        if (transformType == GLOBAL)
        {
            localTransform.Set(globalTransform - parentTransform);
        }
    }

    return positionValueChanged || rotationValueChanged || scaleValueChanged;
}

void EditorUIModule::RenderBasicTransformModifiers(Transform &outputTransform, bool& lockScaleAxis, bool& positionValueChanged, bool& rotationValueChanged, bool& scaleValueChanged)
{
    static bool bUseRad = true;
    if (!bUseRad)
    {
        outputTransform.rotation *= RAD_DEGREE_CONV;
    }
    
    positionValueChanged |= ImGui::InputFloat3( "Position", &outputTransform.position[0] );
    rotationValueChanged |= ImGui::InputFloat3( "Rotation", &outputTransform.rotation[0] );
    if (!bUseRad)
    {
        outputTransform.rotation /= RAD_DEGREE_CONV;
    }
    ImGui::SameLine();
    ImGui::Checkbox("Radians", &bUseRad);
    scaleValueChanged |= ImGui::InputFloat3( "Scale", &outputTransform.scale[0] );
    ImGui::SameLine();
    ImGui::Checkbox("Lock axis", &lockScaleAxis);
}

UID EditorUIModule::RenderResourceSelectDialog(const char* id, const std::unordered_map<std::string, UID> &availableResources)
{
    UID result = CONSTANT_EMPTY_UID;
    if (ImGui::BeginPopup(id))
    {
        static char searchText[255] = "";
        ImGui::InputText("Search", searchText, 255);
        
        ImGui::Separator();
        if (ImGui::BeginListBox("##ComponentList", ImVec2(-FLT_MIN, 5 * ImGui::GetTextLineHeightWithSpacing())))
        {
            for ( const auto &valuePair: availableResources ) {
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

void EditorUIModule::EditorSettings(bool &editorSettingsMenu)
{
    if (!ImGui::Begin("Editor settings", &editorSettingsMenu))
    {
        ImGui::End();
        return;
    }

    ImGui::SeparatorText("Ms and Fps Graph");
    FramePlots();
    ImGui::Spacing();

    ImGui::SeparatorText("Modules Configuration");
    if (ImGui::CollapsingHeader("Window"))
    {
        WindowConfig();
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

    ImGui::End();
}

void EditorUIModule::FramePlots()
{
    char title[25];
    std::vector<float> frametimeVector(frametime.begin(), frametime.end());
    sprintf_s(title, 25, "Milliseconds %0.1f", frametime.back());
    ImGui::PlotHistogram(
        "##milliseconds", &frametimeVector[0], (int)frametimeVector.size(), 0, title, 0.0f, 40.0f, ImVec2(310, 100)
    );

    std::vector<float> framerateVector(framerate.begin(), framerate.end());
    sprintf_s(title, 25, "Framerate %.1f", framerate.back());
    ImGui::PlotHistogram(
        "##framerate", &framerateVector.front(), (int)framerateVector.size(), 0, title, 0.0f, 200.0f, ImVec2(310, 100)
    );
}

void EditorUIModule::WindowConfig()
{
    static bool borderless   = false;
    static bool full_desktop = false;
    static bool resizable    = true;
    static bool fullscreen   = false;

    // Brightness Slider
    float brightness         = App->GetWindowModule()->GetBrightness();
    if (ImGui::SliderFloat("Brightness", &brightness, 0, 1)) App->GetWindowModule()->SetBrightness(brightness);

    SDL_DisplayMode &displayMode = App->GetWindowModule()->GetDesktopDisplayMode();
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
}

void EditorUIModule::CameraConfig() {}

void EditorUIModule::OpenGLConfig()
{
    OpenGLModule *openGLModule = App->GetOpenGLModule();

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