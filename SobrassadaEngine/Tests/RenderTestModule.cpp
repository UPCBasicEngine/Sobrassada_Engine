#include "RenderTestModule.h"

#include "Application.h"
#include "CameraModule.h"
#include "ShaderModule.h"
#include "TextureModuleTest.h"
#include "WindowModule.h"
#include "Framebuffer.h"
#include "OpenGLModule.h"
#include "Components/ComponentMaterial.h"
#include "LightsConfig.h"
#include "DebugDrawModule.h"
#include "WindowModule.h"

#include "MathGeoLib.h"
#include "glew.h"
#include "DirectXTex/DirectXTex.h"
#include "EngineModel.h"
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"

RenderTestModule::RenderTestModule()
{
	currentLoadedModel = new EngineModel();
    lightsConfig       = new LightsConfig();
}

RenderTestModule::~RenderTestModule()
{
}

bool RenderTestModule::Init()
{
	//currentLoadedModel->Load("./Test/BakerHouse.gltf");
	currentLoadedModel->Load("./Test/Robot.gltf");	
	
	//program = App->GetShaderModule()->GetProgram("./Test/basicVertexShader.vs", "./Test/basicFragmentShader.fs");
    program = App->GetShaderModule()->GetProgram("./Test/VertexShader.glsl", "./Test/BRDFPhongFragmentShader.glsl");
	//program = App->GetShaderModule()->GetProgram("./Test/VertexShader.glsl", "./Test/FragmentShader.glsl");

	// Triangle without UV's
	/*float vtx_data[] = { 
		-1.0f, -1.0f, 0.0f, 
		1.0f, -1.0f, 0.0f, 
		0.0f, 1.0f, 0.0f,
	};*/

	// Texture loading
	DirectX::TexMetadata textureMetadata;
	auto baboonPath = L"./Test/baboon.ppm";
	baboonTexture = App->GetTextureModuleTest()->LoadTexture(baboonPath, textureMetadata);
	
	// Quad with UV's
	float vtx_data[] = {
		-1.0f, 1.0f, 0.0f,
		-1.0f, -1.0f, 0.0f,
		 1.0f, -1.0f, 0.0f,

		1.0f, -1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,
		-1.0f, 1.0f, 0.0f,

		0.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 1.0f,

		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f
	};

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vtx_data), vtx_data, GL_STATIC_DRAW);

	lightsConfig->InitSkybox();

	return true;
}

update_status RenderTestModule::PreUpdate(float deltaTime)
{
	return UPDATE_CONTINUE;
}

update_status RenderTestModule::Render(float deltaTime)
{
	float4x4 model, view, proj;

	proj = App->GetCameraModule()->GetProjectionMatrix();
	view = App->GetCameraModule()->GetViewMatrix();
	App->GetCameraModule()->UpdateUBO();
	model = float4x4::FromTRS(
		float3(2.0f, 0.0f, 0.0f),
		float4x4::RotateZ(pi / 4.0f),
		float3(2.0f, 1.0f, 1.0f));

	// Draw the skybox before anything else
	lightsConfig->RenderSkybox(proj, view);					

	glUseProgram(program);
	//glUniformMatrix4fv(0, 1, GL_TRUE, &proj[0][0]);
	//glUniformMatrix4fv(1, 1, GL_TRUE, &view[0][0]);
	//glUniformMatrix4fv(2, 1, GL_TRUE, &model[0][0]);

	// Sending triangle vertices
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

	// Sending texture coordiantes
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)(sizeof(float) * 3 * 6));

    float3 cameraPos = App->GetCameraModule()->getPosition();
    glUniform3fv(glGetUniformLocation(program, "cameraPos"), 1, &cameraPos[0]);

    if (ImGui::CollapsingHeader("Light"))
    {
		ImGui::SliderFloat3("Ambient Intensity", &ambientIntensity.x, 0.f, 1.f);
		ImGui::SliderFloat3("Light Color", &lightColor.x, 0.f, 1.f);
        ImGui::SliderFloat3("Light Direction", &lightDir.x, -20.0f, 20.0f);
    }

    glUniform3fv(glGetUniformLocation(program, "lightDir"), 1, &lightDir[0]);
	glUniform3fv(glGetUniformLocation(program, "lightColor"), 1, &lightColor[0]);
    glUniform3fv(glGetUniformLocation(program, "ambientIntensity"), 1, &ambientIntensity[0]);

    materials = currentLoadedModel->GetMaterials();
    materials.at(0)->OnEditorUpdate();

	//glActiveTexture(GL_TEXTURE0);
	//glBindTexture(GL_TEXTURE_2D, baboonTexture);

	//glDrawArrays(GL_TRIANGLES, 0, 3);
	//glDrawArrays(GL_TRIANGLES, 0, 6);

	unsigned int cameraUBO = App->GetCameraModule()->GetUbo();
	currentLoadedModel->Render(program, cameraUBO);

	int width  = 0;
    int height = 0;
    SDL_GetWindowSize(App->GetWindowModule()->window, &width, &height);

	// Draw the grid at last
	App->GetDebugDreawModule()->Draw(view, proj, width, height);

	// Unbinding frame buffer so ui gets rendered
	App->GetOpenGLModule()->GetFramebuffer()->Unbind();

	return UPDATE_CONTINUE;
}

bool RenderTestModule::ShutDown()
{
	App->GetShaderModule()->DeleteProgram(program);
	glDeleteBuffers(1, &vbo);
	glDeleteTextures(1, &baboonTexture);

	delete currentLoadedModel;
    delete lightsConfig;

	return true;
}

void RenderTestModule::RenderEditorViewport()
{
	ImGui::SetNextWindowSize(ImVec2(500, 500));
	ImGui::SetNextWindowPos(ImVec2(0,50));
	ImGui::Begin("Scene");
	{
		ImGui::BeginChild("GameRender");

		float width = ImGui::GetContentRegionAvail().x;
		float height = ImGui::GetContentRegionAvail().y;

		ImGui::Image(
			(ImTextureID)0,
			ImGui::GetContentRegionAvail(),
			ImVec2(0, 1),
			ImVec2(1, 0)
		);
	}
	ImGui::EndChild();
	ImGui::End();
}