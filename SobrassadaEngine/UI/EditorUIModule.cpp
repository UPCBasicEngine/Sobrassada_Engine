#include "EditorUIModule.h"

#include "Application.h"
#include "EditorViewport.h"
#include "OpenGLModule.h"
#include "WindowModule.h"
#include "FileSystem.h"

#include "glew.h"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl2.h"
#include <filesystem>

EditorUIModule::EditorUIModule()
{
	startPath = std::filesystem::current_path().string();
}

EditorUIModule::~EditorUIModule() {}

bool EditorUIModule::Init()
{
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
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

	accPaths.clear();
	files.clear();

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
	ImGui::DockSpaceOverViewport();
	MainMenu();
	ImGui::ShowDemoWindow();

	if (consoleMenu) Console(consoleMenu);

	if (import) ImportDialog(import);

	if (editorSettingsMenu) EditorSettings(editorSettingsMenu);
}

void EditorUIModule::MainMenu()
{
	ImGui::BeginMainMenuBar();

	// General menu
	if (ImGui::BeginMenu("General"))
	{
		if (ImGui::MenuItem("Console")) consoleMenu = !consoleMenu;

		if (ImGui::MenuItem("Import")) import = !import;

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

void EditorUIModule::ImportDialog(bool& import)
{
	ImGui::Begin("Import Dialog", & import);

	static std::string currentPath = std::filesystem::current_path().string();

	// root directory add delimiter
	if (currentPath.back() == ':') currentPath += DELIMITER;

	FileSystem::SplitAccumulatedPath(currentPath, accPaths);

	for (size_t i = 0; i < accPaths.size(); i++)
	{
		const std::string& accPath = accPaths[i];

		std::string buttonLabel = (i == 0) ? accPath : FileSystem::GetFileNameWithExtension(accPath);

		if (ImGui::Button(buttonLabel.c_str()))
		{
			currentPath = accPath;
		}

		if (i < accPaths.size() -1) ImGui::SameLine();
	}

	GetFilesSorted(currentPath);

	static std::string inputFile = "";
	static int selected = -1;

	for (int i = 0; i < files.size(); i++)
	{
		const std::string& file = files[i];
		std::string filePath = currentPath + DELIMITER + file;
		bool isDirectory = FileSystem::IsDirectory(filePath.c_str());

		std::string tag = isDirectory ? "[Dir] " : "[File] ";
		std::string fileWithTag = tag + file;

		// ImGuiSelectableFlags_AllowDoubleClick for Directories?
		if (ImGui::Selectable(fileWithTag.c_str(), selected == i))
		{
			selected = i;

			if (file == "..")
			{
				currentPath = FileSystem::GetParentPath(currentPath);
				inputFile = "";
				selected = -1;
				GetFilesSorted(currentPath);
			}
			else if (isDirectory)
			{
				currentPath = filePath;
				inputFile = "";
				selected = -1;
				GetFilesSorted(currentPath);
			}
			else
			{
				inputFile = FileSystem::GetFileNameWithExtension(file);
			}
		}
	}

	ImGui::Dummy(ImVec2(0, ImGui::GetContentRegionAvail().y - 50));
	ImGui::Text("File Name:");
	ImGui::SameLine();
	ImGui::InputText("##filename", &inputFile[0], inputFile.size(), ImGuiInputTextFlags_ReadOnly);

	if (ImGui::Button("Cancel", ImVec2(0, 0)))
	{
		inputFile = "";
		currentPath = startPath;
		import = false;
	}

	ImGui::SameLine();

	if (ImGui::Button("Ok", ImVec2(0, 0)))
	{
		if (!inputFile.empty())
		{
			std::string importPath = currentPath + DELIMITER + inputFile;
			// Call to SceneImporter import
		}
		import = false;
	}

	ImGui::End();
}

void EditorUIModule::GetFilesSorted(const std::string& currentPath)
{
	//files & dir in the current directory
	FileSystem::GetAllInDirectory(currentPath, files);

	files.insert(files.begin(), "..");

	std::sort(files.begin(), files.end(), [&](const std::string& a, const std::string& b) {
		bool isDirA = FileSystem::IsDirectory((currentPath + DELIMITER + a).c_str());
		bool isDirB = FileSystem::IsDirectory((currentPath + DELIMITER + b).c_str());

		if (isDirA != isDirB) return isDirA > isDirB;
		return a < b;
	});
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
	static bool borderless = false;
	static bool full_desktop = false;
	static bool resizable = true;
	static bool fullscreen = false;

	// Brightness Slider
	float brightness = App->GetWindowModule()->GetBrightness();
	if (ImGui::SliderFloat("Brightness", &brightness, 0, 1)) App->GetWindowModule()->SetBrightness(brightness);

	SDL_DisplayMode& displayMode = App->GetWindowModule()->GetDesktopDisplayMode();
	int maxWidth = displayMode.w;
	int maxHeight = displayMode.h;

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

void EditorUIModule::CameraConfig() {}

void EditorUIModule::OpenGLConfig()
{
	OpenGLModule* openGLModule = App->GetOpenGLModule();

	float clearRed = openGLModule->GetClearRed();
	float clearGreen = openGLModule->GetClearGreen();
	float clearBlue = openGLModule->GetClearBlue();

	if (ImGui::SliderFloat("Red Clear Color", &clearRed, 0.f, 1.f)) openGLModule->SetClearRed(clearRed);
	if (ImGui::SliderFloat("Green Clear Color", &clearGreen, 0.f, 1.f)) openGLModule->SetClearGreen(clearGreen);
	if (ImGui::SliderFloat("Blue Clear Color", &clearBlue, 0.f, 1.f)) openGLModule->SetClearBlue(clearBlue);

	if (ImGui::Button("Reset Colors"))
	{
		clearRed = DEFAULT_GL_CLEAR_COLOR_RED;
		clearGreen = DEFAULT_GL_CLEAR_COLOR_GREEN;
		clearBlue = DEFAULT_GL_CLEAR_COLOR_BLUE;

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
	bool changed = false;
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