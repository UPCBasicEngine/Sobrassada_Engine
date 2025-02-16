#include "EditorViewport.h"

#include "Application.h"
#include "OpenGLModule.h"
#include "Framebuffer.h"

#include "imgui.h"

// TODO: THIS MODULE WILL BE REMOVED
#include "CameraModule.h"
#include "EditorUIModule.h"

EditorViewport::EditorViewport()
{
}

EditorViewport::~EditorViewport()
{
}

void EditorViewport::Render()
{
	if (ImGui::Begin("Sence"))
	{
	    
		if (ImGui::BeginChild("##SceneChild", ImVec2(0.f, 0.f), NULL, ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar))
		{
			const auto& framebuffer = App->GetOpenGLModule()->GetFramebuffer();

			ImGui::SetCursorPos(ImVec2(0.f, 0.f));

			ImGui::Image(
				(ImTextureID)framebuffer->GetTextureID(),
				ImVec2((float)framebuffer->GetTextureWidth(), (float)framebuffer->GetTextureHeight()),
				ImVec2(0.f, 1.f),
				ImVec2(1.f, 0.f)
			);

		    ImGuizmo::SetOrthographic(false);
		    ImGuizmo::SetDrawlist(); // ImGui::GetWindowDrawList()

		    float width = ImGui::GetWindowWidth();
		    float height = ImGui::GetWindowHeight();
		    ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, width, height);
		    
		   ImGui::EndChild();

			ImVec2 windowSize = ImGui::GetWindowSize();
			if (framebuffer->GetTextureWidth() != windowSize.x || framebuffer->GetTextureHeight() != windowSize.y)
			{
				float aspectRatio = windowSize.y / windowSize.x;
				App->GetCameraModule()->SetAspectRatio(aspectRatio);
				framebuffer->Resize((int)windowSize.x, (int)windowSize.y);
			}
		}
	}
	ImGui::End();
}
