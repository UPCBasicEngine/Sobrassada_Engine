#include "EditorUIModule.h"

#include "Application.h"
#include "EditorViewport.h"
#include "GameTimer.h"
#include "InputModule.h"
#include "OpenGLModule.h"
#include "WindowModule.h"

#include "glew.h"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl2.h"
#include <string>

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
    ImGui::DockSpaceOverViewport();
    MainMenu();

    if (consoleMenu) Console(consoleMenu);

    if (aboutMenu) About(aboutMenu);

    if (editorSettingsMenu) EditorSettings(editorSettingsMenu);
}

void EditorUIModule::MainMenu()
{
    ImGui::BeginMainMenuBar();

    // General menu
    if (ImGui::BeginMenu("General"))
    {
        if (ImGui::MenuItem("Console")) consoleMenu = !consoleMenu;

        if (ImGui::MenuItem("About")) aboutMenu = !aboutMenu;

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

void EditorUIModule::About(bool &aboutMenu) const
{
    std::string title = "About " + std::string(ENGINE_NAME);
    ImGui::Begin(title.c_str(), &aboutMenu);

    ImGui::Text("%s (%s)", ENGINE_NAME, ENGINE_VERSION);

    // TODO:
    // update needed when github project renamed
    // no license at the moment
    ImGui::TextLinkOpenURL("GitHub", "https://github.com/UPCBasicEngine/Sobrassada_Engine");
    ImGui::SameLine();
    ImGui::TextLinkOpenURL("Readme", "https://github.com/UPCBasicEngine/Sobrassada_Engine/blob/main/README.md");
    ImGui::SameLine();
    ImGui::TextLinkOpenURL("License", "https://github.com/UPCBasicEngine/Sobrassada_Engine/blob/main/README.md");
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
        ImGuiIO &io            = ImGui::GetIO();
        ImGuiStyle &style      = ImGui::GetStyle();

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

void EditorUIModule::EditorSettings(bool &editorSettingsMenu)
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
        InputModule *inputModule = App->GetInputModule();
        const KeyState *keyboard = inputModule->GetKeyboard();
        ImGui::Text("Key Q: %s", (keyboard[SDL_SCANCODE_Q] ? "Pressed" : "Not Pressed"));
        ImGui::Text("Key E: %s", (keyboard[SDL_SCANCODE_E] ? "Pressed" : "Not Pressed"));
        ImGui::Text("Key W: %s", (keyboard[SDL_SCANCODE_W] ? "Pressed" : "Not Pressed"));
        ImGui::Text("Key A: %s", (keyboard[SDL_SCANCODE_A] ? "Pressed" : "Not Pressed"));
        ImGui::Text("Key S: %s", (keyboard[SDL_SCANCODE_S] ? "Pressed" : "Not Pressed"));
        ImGui::Text("Key D: %s", (keyboard[SDL_SCANCODE_D] ? "Pressed" : "Not Pressed"));
        ImGui::Text("Key LSHIFT: %s", (keyboard[SDL_SCANCODE_LSHIFT] ? "Pressed" : "Not Pressed"));
        ImGui::Text("Key LALT: %s", (keyboard[SDL_SCANCODE_LALT] ? "Pressed" : "Not Pressed"));

        const KeyState *mouseButtons = inputModule->GetMouseButtons();
        ImGui::Text("Mouse Left: %s", (mouseButtons[SDL_BUTTON_LEFT - 1] ? "Pressed" : "Not Pressed"));
        ImGui::Text("Mouse Right: %s", (mouseButtons[SDL_BUTTON_RIGHT - 1] ? "Pressed" : "Not Pressed"));
        ImGui::Text("Mouse Middle: %s", (mouseButtons[SDL_BUTTON_MIDDLE - 1] ? "Pressed" : "Not Pressed"));

        const float2 &mousePos = inputModule->GetMousePosition();
        ImGui::Text("Mouse Position: (%.f, %.f)", mousePos.x, mousePos.y);

        const float2 & mouseMotion = inputModule->GetMouseMotion();
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

void EditorUIModule::WindowConfig(bool &vsync) const
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

    const char *version = reinterpret_cast<const char *>(glGetString(GL_VERSION));
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

    const char *vendor = reinterpret_cast<const char *>(glGetString(GL_VENDOR));
    ImGui::Text("GPU:");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%s", vendor);

    const char *renderer = reinterpret_cast<const char *>(glGetString(GL_RENDERER));
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
    for (const auto &feature : features)
    {
        if (feature.check() == SDL_TRUE)
        {
            if (cont != 4) ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%s", feature.name);
            cont++;
        }
    }
}