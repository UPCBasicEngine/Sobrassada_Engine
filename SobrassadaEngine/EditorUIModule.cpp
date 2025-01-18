#include "EditorUIModule.h"

#include "Application.h"
#include "WindowModule.h"
#include "OpenGLModule.h"

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"


EditorUIModule::EditorUIModule()
{
}

EditorUIModule::~EditorUIModule()
{
}

bool EditorUIModule::Init()
{
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // IF using Docking Branch
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

	ImGui_ImplSDL2_InitForOpenGL(App->GetWindowModule()->window, App->GetOpenGLModule()->GetContext());
	ImGui_ImplOpenGL3_Init("#version 460");

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
	AddFramePlotData(deltaTime);

	Draw();

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	// Needed for the viewports flag to work
	if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		SDL_Window* backup_current_window = SDL_GL_GetCurrentWindow();
		SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
		SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
	}

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

void EditorUIModule::Console(bool& consoleMenu)
{
	ImGui::Begin("Console", &consoleMenu);

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

void EditorUIModule::EditorSettings(bool& editorSettingsMenu)
{
	ImGui::Begin("Editor settings", &editorSettingsMenu);
	
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
		//CameraConfig();
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
	ImGui::PlotHistogram("##milliseconds", &frametimeVector[0], (int)frametimeVector.size(), 0, title, 0.0f, 40.0f, ImVec2(310, 100));

	std::vector<float> framerateVector(framerate.begin(), framerate.end());
	sprintf_s(title, 25, "Framerate %.1f", framerate.back());
	ImGui::PlotHistogram("##framerate", &framerateVector.front(), (int)framerateVector.size(), 0, title, 0.0f, 200.0f, ImVec2(310, 100));
}

void EditorUIModule::WindowConfig()
{
	static bool borderless = false;
	static bool full_desktop = false;
	static bool resizable = true;
	static bool fullscreen = false;

	// Brightness Slider
	float brightness = App->GetWindowModule()->GetBrightness();
	if (ImGui::SliderFloat("Brightness", &brightness, 0, 1)) App->GetWindowModule()->SetBrightness(brightness);

	SDL_DisplayMode& displayMode = App->GetWindowModule()->GetDesktopDisplayMode();
	int maxWidth = displayMode.w;
	int maxHeight= displayMode.h;

	// Width Slider
	int width = App->GetWindowModule()->GetWidth();
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

void EditorUIModule::CameraConfig()
{
}

void EditorUIModule::OpenGLConfig()
{
}
