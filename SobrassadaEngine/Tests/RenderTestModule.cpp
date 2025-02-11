#include "RenderTestModule.h"

#include "Application.h"
#include "CameraModule.h"
#include "ShaderModule.h"
#include "TextureModuleTest.h"
#include "WindowModule.h"
#include "Framebuffer.h"
#include "OpenGLModule.h"

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
}

RenderTestModule::~RenderTestModule()
{
}

bool RenderTestModule::Init()
{
	currentLoadedModel->Load("./Test/BakerHouse.gltf");

	//program = App->GetShaderModule()->GetProgram("./Test/basicVertexShader.vs", "./Test/basicFragmentShader.fs");
	program = App->GetShaderModule()->GetProgram("./Test/VertexShader.glsl", "./Test/FragmentShader.glsl");

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

	// Sending texture coordiantes
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)(sizeof(float) * 3 * 6));

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, baboonTexture);

	//glDrawArrays(GL_TRIANGLES, 0, 3);
	glDrawArrays(GL_TRIANGLES, 0, 6);

    bool insideFrustum = CheckFrustum(proj, view, model);

	if (insideFrustum)
    {
       //GLOG("El objeto esta dentro del Frustum");
        currentLoadedModel->Render(program, proj, view);
    }
    else
    {
       // GLOG("El objeto esta fuera del Frustum");
    }
	

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

bool RenderTestModule::CheckFrustum(const math::float4x4 &proj, const math::float4x4 &view, const math::float4x4 &model)
{
    OBB currentModelObb = currentLoadedModel->GetOBBModel();
    float3 corners[8];
    currentModelObb.GetCornerPoints(corners);
    math::float4x4 VP = proj * view;

    ExtractFrustumPlanes(VP, frustumPlanes);

    for (int j = 0; j < 6; ++j)
    {
        bool allOutside = true;

        for (int i = 0; i < 8; ++i)
        {
            if (PointInPlane(corners[i], frustumPlanes[j]))
            {
                allOutside = false;
                break;
            }
        }

        if (allOutside) return false;
    }

    return true;
}

void RenderTestModule::ExtractFrustumPlanes(const float4x4 &vpMatrix, float4 planes[6])
{
    for (int i = 0; i < 6; ++i)
    {

        // Plano Izquierda
        if (i == 0)
        {
            planes[i].x = vpMatrix[3][0] + vpMatrix[0][0];
            planes[i].y = vpMatrix[3][1] + vpMatrix[0][1];
            planes[i].z = vpMatrix[3][2] + vpMatrix[0][2];
            planes[i].w = vpMatrix[3][3] + vpMatrix[0][3];
        }
        // Plano Derecha
        else if (i == 1)
        {
            planes[i].x = vpMatrix[3][0] - vpMatrix[0][0];
            planes[i].y = vpMatrix[3][1] - vpMatrix[0][1];
            planes[i].z = vpMatrix[3][2] - vpMatrix[0][2];
            planes[i].w = vpMatrix[3][3] - vpMatrix[0][3];
        }
        // Plano abajo
        else if (i == 2)
        {
            planes[i].x = vpMatrix[3][0] + vpMatrix[1][0];
            planes[i].y = vpMatrix[3][1] + vpMatrix[1][1];
            planes[i].z = vpMatrix[3][2] + vpMatrix[1][2];
            planes[i].w = vpMatrix[3][3] + vpMatrix[1][3];
        }
        // Plano arriba
        else if (i == 3)
        {
            planes[i].x = vpMatrix[3][0] - vpMatrix[1][0];
            planes[i].y = vpMatrix[3][1] - vpMatrix[1][1];
            planes[i].z = vpMatrix[3][2] - vpMatrix[1][2];
            planes[i].w = vpMatrix[3][3] - vpMatrix[1][3];
        }
        // Plano Near
        else if (i == 4)
        {
            planes[i].x = vpMatrix[3][0] + vpMatrix[2][0];
            planes[i].y = vpMatrix[3][1] + vpMatrix[2][1];
            planes[i].z = vpMatrix[3][2] + vpMatrix[2][2];
            planes[i].w = vpMatrix[3][3] + vpMatrix[2][3];
        }
        // Plano Far
        else if (i == 5)
        {
            planes[i].x = vpMatrix[3][0] - vpMatrix[2][0];
            planes[i].y = vpMatrix[3][1] - vpMatrix[2][1];
            planes[i].z = vpMatrix[3][2] - vpMatrix[2][2];
            planes[i].w = vpMatrix[3][3] - vpMatrix[2][3];
        }

        planes[i].Normalize4();
    }
}

bool RenderTestModule::PointInPlane(const float3 &point, const float4 &plane)
{
    return (plane.x * point.x + plane.y * point.y + plane.z * point.z + plane.w) >= 0.0f;
}