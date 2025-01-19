#pragma once

#include "Module.h"

class RenderTestModule : public Module
{
public:
	RenderTestModule();
	~RenderTestModule();

	bool Init() override;
	update_status Render(float deltaTime) override;
	bool ShutDown() override;

private:
	
	int program = -1;
	unsigned vbo = -1;
	unsigned int baboonTexture = -1;
};

