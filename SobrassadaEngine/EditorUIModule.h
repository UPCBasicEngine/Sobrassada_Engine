#pragma once

#include "Module.h"

#include<deque>

class EditorUIModule : public Module
{
public:
	EditorUIModule();
	~EditorUIModule();

	bool Init() override;
	update_status PreUpdate(float deltaTime) override;
	update_status Update(float deltaTime) override;
	bool ShutDown() override;

private:

	void AddFramePlotData(float deltaTime);
	void Draw();

	void MainMenu();
	void EditorSettings(bool& editorSettingsMenu);
	
	void FramePlots();
	void WindowConfig();
	void CameraConfig();
	void OpenGLConfig();

	void Console(bool& consoleMenu);

	bool consoleMenu = false;
	bool editorSettingsMenu = false;
	bool closeApplication = false;

	int maximumPlotData = 50;
	std::deque<float> framerate;
	std::deque<float> frametime;
};

