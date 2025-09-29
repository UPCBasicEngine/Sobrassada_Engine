#include "GameDebugUIModule.h"

#include "Application.h"
#include "DebugDrawModule.h"
#include "EditorUIModule.h"
#include "GameTimer.h"
#include "InputModule.h"
#include "OpenGLModule.h"
#include "PhysicsModule.h"

#include "SDL.h"
#include "glew.h"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl2.h"
#include "imgui_internal.h"

// TESTING
#include "CameraComponent.h"
#include "Framebuffer.h"
#include "Geometry/LineSegment.h"
#include "SceneModule.h"
#include "btVector3.h"

GameDebugUIModule::GameDebugUIModule()
{
}

GameDebugUIModule::~GameDebugUIModule()
{
}

bool GameDebugUIModule::Init()
{
    return true;
}

update_status GameDebugUIModule::PreUpdate(float deltaTime)
{
#ifdef GAME

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

#endif

    return UPDATE_CONTINUE;
}

update_status GameDebugUIModule::Update(float deltaTime)
{
#ifdef GAME
    if (App->GetInputModule()->GetKeyboard()[SDL_SCANCODE_F1] == KeyState::KEY_DOWN)
    {
        gameDebugMenu = !gameDebugMenu;
    }

#endif
    return UPDATE_CONTINUE;
}

update_status GameDebugUIModule::RenderEditor(float deltaTime)
{

#ifdef GAME
    Draw();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

#endif
    return UPDATE_CONTINUE;
}

update_status GameDebugUIModule::PostUpdate(float deltaTime)
{
#ifdef GAME
    if (closeApplication) return UPDATE_STOP;
#endif

    return UPDATE_CONTINUE;
}

bool GameDebugUIModule::ShutDown()
{
    return true;
}

void GameDebugUIModule::Draw()
{
    if (gameDebugMenu) GameDebugMenu();
    RenderOptions();
}

void GameDebugUIModule::GameDebugMenu()
{
    if (!ImGui::Begin("GameDebug", &gameDebugMenu))
    {
        ImGui::End();
        return;
    }

    // Delta time comes in ms instead of s
    const float deltaTime = App->GetGameTimer()->GetDeltaTime();
    const float fps       = deltaTime ? 1000.f / deltaTime : 0;

    ImGui::Text("FPS:");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), std::to_string(fps).c_str());

    ImGui::Checkbox("Console", &openConsole);

    if (openConsole) App->GetEditorUIModule()->Console(openConsole);

    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Spacing();

    FXAAParameters fxaa = App->GetSceneModule()->GetScene()->GetRenderPass()->GetFXAAParameters();
    ImGui::Checkbox("Enable FXAA", &fxaa.isEnabled);
    ImGui::Checkbox("Show borders", &fxaa.showBorders);
    App->GetSceneModule()->GetScene()->GetRenderPass()->SetFXAAParameters(fxaa);

    ImGui::Separator();
    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Spacing();

    if (ImGui::Button("Close Game")) closeApplication = true;

    ImGui::End();
}

void GameDebugUIModule::RenderOptions()
{
    if (App->GetInputModule()->GetKeyboard()[SDL_SCANCODE_F2])
    {
        ImGui::OpenPopup("RenderOptions");
    }

    if (ImGui::BeginPopup("RenderOptions"))
    {
        int stringCount   = sizeof(DebugStrings) / sizeof(char*);
        float listBoxSize = (float)stringCount + 0.5f;
        if (ImGui::BeginListBox(
                "##RenderOptionsList", ImVec2(ImGui::CalcItemWidth(), ImGui::GetFrameHeightWithSpacing() * listBoxSize)
            ))
        {
            const auto& debugBitset = App->GetDebugDrawModule()->GetDebugOptionValues();
            for (int i = 0; i < stringCount; ++i)
            {
                bool currentBitValue = debugBitset[i];
                if (ImGui::Checkbox(DebugStrings[i], &currentBitValue))
                {
                    App->GetDebugDrawModule()->FlipDebugOptionValue(i);
                    if (i == (int)DebugOptions::RENDER_WIREFRAME)
                        App->GetOpenGLModule()->SetRenderWireframe(currentBitValue);
                    else if (i == (int)DebugOptions::RENDER_PHYSICS_WORLD)
                        App->GetPhysicsModule()->SetDebugOption(currentBitValue);
                }
            }

            ImGui::EndListBox();
        }

        ImGui::EndPopup();
    }
}