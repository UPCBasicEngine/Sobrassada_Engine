#include "RenderTestModule.h"

#include "Application.h"
#include "CameraModule.h"
#include "ShaderModule.h"

#include "MathGeoLib.h"
#include "glew.h"

RenderTestModule::RenderTestModule()
{
}

RenderTestModule::~RenderTestModule()
{
}

bool RenderTestModule::Init()
{
	program = App->GetShaderModule()->GetProgram("./basicVertexShader.vs", "./basicFragmentShader.fs");

	float vtx_data[] = { 
		-1.0f, -1.0f, 0.0f, 
		1.0f, -1.0f, 0.0f, 
		0.0f, 1.0f, 0.0f,
	};

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vtx_data), vtx_data, GL_STATIC_DRAW);

	return true;
}

update_status RenderTestModule::Render(float deltaTime)
{

	float4x4 model, view, proj;

	proj = App->GetCameraModule()->GetProjectionMatrix();
	view = App->GetCameraModule()->GetViewMatrix();
	model = float4x4::FromTRS(
		float3(2.0f, 0.0f, 0.0f),
		float4x4::RotateZ(pi / 4.0f),
		float3(2.0f, 1.0f, 1.0f));
					

	glUseProgram(program);
	glUniformMatrix4fv(0, 1, GL_TRUE, &proj[0][0]);
	glUniformMatrix4fv(1, 1, GL_TRUE, &view[0][0]);
	glUniformMatrix4fv(2, 1, GL_TRUE, &model[0][0]);

	// Sending triangle vertices
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

	glDrawArrays(GL_TRIANGLES, 0, 3);

	return UPDATE_CONTINUE;
}

bool RenderTestModule::ShutDown()
{
	App->GetShaderModule()->DeleteProgram(program);
	glDeleteBuffers(1, &vbo);
	return true;
}
