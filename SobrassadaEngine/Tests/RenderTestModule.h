#pragma once

#include "Module.h"
#include "MathGeoLib.h"

class EngineModel;

class RenderTestModule : public Module
{
public:
	RenderTestModule();
	~RenderTestModule();

	bool Init() override;
	update_status PreUpdate(float deltaTime) override;
	update_status Render(float deltaTime) override;
	bool ShutDown() override;

	int GetProgram() const { return program; }
private:
	int program = -1;
	unsigned int vbo = -1;
	unsigned int baboonTexture = -1;

	EngineModel* currentLoadedModel;
    
};

