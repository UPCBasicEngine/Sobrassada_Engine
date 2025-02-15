#include "EditorUIModule.h"

#include "Application.h"
#include "EditorViewport.h"
#include "GameTimer.h"
#include "OpenGLModule.h"
#include "WindowModule.h"

#include "glew.h"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl2.h"

EditorUIModule::EditorUIModule() {}

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

    return true;
}

update_status EditorUIModule::PreUpdate(float deltaTime)
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

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

void EditorUIModule::LimitFPS(float deltaTime)
{
    if (deltaTime == 0) return;

    float targetFrameTime = 1000.f / maxFPS;

    if (maxFPS != 0)
    {
        if (deltaTime < targetFrameTime) SDL_Delay(targetFrameTime - deltaTime);
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
    ImGui::DockSpaceOverViewport();
    MainMenu();

    if (consoleMenu) Console(consoleMenu);

    if (editorSettingsMenu) EditorSettings(editorSettingsMenu);
}

void EditorUIModule::MainMenu()
{
    ImGui::BeginMainMenuBar();

    // General menu
    if (ImGui::BeginMenu("General"))
    {
        if (ImGui::MenuItem("Console")) consoleMenu = !consoleMenu;

        if (ImGui::MenuItem("Quit")) closeApplication = true;

        ImGui::EndMenu();
    }

    // Settings menu
    if (ImGui::BeginMenu("Settings"))
    {
        if (ImGui::MenuItem("Editor settings")) editorSettingsMenu = !editorSettingsMenu;

        ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
}

void EditorUIModule::Console(bool &consoleMenu) const
{
    ImGui::Begin("Console", &consoleMenu);

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

void EditorUIModule::EditorSettings(bool &editorSettingsMenu) const
{
    ImGui::Begin("Editor settings", &editorSettingsMenu);

    static bool vsync = VSYNC;

    if (ImGui::CollapsingHeader("Application"))
    {
        ImGui::SeparatorText("Information");
        ImGui::InputText(
            "App Name", const_cast<char *>(ENGINE_NAME), IM_ARRAYSIZE(ENGINE_NAME), ImGuiInputTextFlags_ReadOnly
        );
        ImGui::InputText(
            "Organization", const_cast<char *>(ORGANIZATION_NAME), IM_ARRAYSIZE(ORGANIZATION_NAME),
            ImGuiInputTextFlags_ReadOnly
        );

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
    if (ImGui::CollapsingHeader("Game timer"))
    {
        GameTimerConfig();
    }

    ImGui::End();
}

void EditorUIModule::FramePlots(bool &vsync)
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

void EditorUIModule::WindowConfig(bool &vsync)
{
    static bool borderless   = BORDERLESS;
    static bool full_desktop = FULL_DESKTOP;
    static bool resizable    = RESIZABLE;
    static bool fullscreen   = FULLSCREEN;

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

    // Set Vsync
    if (ImGui::Checkbox("Vsync", &vsync)) App->GetWindowModule()->SetVsync(vsync);
}

void EditorUIModule::CameraConfig() const {}

void EditorUIModule::OpenGLConfig() const
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

void EditorUIModule::GameTimerConfig() const
{
    GameTimer *gameTimer = App->GetGameTimer();
    float timeScale = gameTimer->GetTimeScale();

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