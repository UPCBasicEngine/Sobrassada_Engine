#include "EditorViewport.h"

#include "Application.h"
#include "OpenGLModule.h"
#include "Framebuffer.h"

#include "imgui.h"

EditorViewport::EditorViewport()
{
}

EditorViewport::~EditorViewport()
{
}

void EditorViewport::Render()
{
	static bool openWindow = true;

	if (ImGui::Begin("Sence"), &openWindow)
	{
		if (ImGui::BeginChild("##SceneChild", ImVec2(0.f, 0.f), ImGuiWindowFlags_NoScrollWithMouse))
		{
			const auto& framebuffer = App->GetOpenGLModule()->GetFramebuffer();


			ImGui::SetCursorPos(ImVec2(0.f, 0.f));

			ImGui::Image(
				(ImTextureID)framebuffer->GetTextureID(),
				ImVec2((float)framebuffer->GetTextureWidth(), (float)framebuffer->GetTextureHeight()),
				ImVec2(0.f, 1.f),
				ImVec2(1.f, 0.f)
			);
			ImGui::EndChild();

			ImVec2 windowSize = ImGui::GetWindowSize();
			if (framebuffer->GetTextureWidth() != windowSize.x || framebuffer->GetTextureHeight() != windowSize.y) framebuffer->Resize((int)windowSize.x, (int)windowSize.y);
		}
	}
	ImGui::End();
}
