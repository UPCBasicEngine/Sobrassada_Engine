#include "TextureEditor.h"

#include "Application.h"
#include "LibraryModule.h"
#include "ResourceTexture.h"
#include "ResourcesModule.h"
#include "HashString.h"

#include "imgui.h"

TextureEditor::TextureEditor(const std::string& editorName, UID uid) : EngineEditorBase(editorName, uid)
{
}

TextureEditor::~TextureEditor()
{
}

bool TextureEditor::RenderEditor()
{

    if (!EngineEditorBase::RenderEditor()) return false;
    ImGui::Begin(name.c_str());
    const auto& textureMap = App->GetLibraryModule()->GetTextureMap();

    if (textureMap.empty())
    {
        ImGui::Text("No textures loaded.");
        ImGui::End();
        return true;
    }

    ImGui::Text("Loaded Textures:");
    ImGui::Separator();

    ImGui::Columns(5, "textures", false);
    ImGui::Text("Name");
    ImGui::NextColumn();
    ImGui::Text("Width");
    ImGui::NextColumn();
    ImGui::Text("Height");
    ImGui::NextColumn();
    ImGui::Text("Mipmap Levels");
    ImGui::NextColumn();
    ImGui::Text("Memory Size (KB)");
    ImGui::NextColumn();
    ImGui::Separator();

    for (const auto& [name, textureUID] : textureMap)
    {
        Resource* resource = App->GetResourcesModule()->RequestResource(textureUID);
        if (!resource) continue;

        ResourceTexture* texture = dynamic_cast<ResourceTexture*>(resource);
        if (!texture) continue;

        const int width        = texture->GetTextureWidth();
        const int height       = texture->GetTextureHeight();
        const int mipmapLevels = texture->GetMipMapLevels();
        const int sizeKB       = (width * height * 4) / 1024; //(4 bytes for pixel -> RGBA)

        // Save the UID of selected texture
        if (ImGui::Selectable(name.c_str()))
        {
            selectedTextureUID = textureUID;
            showViewportWindow = true;
        }
        ImGui::NextColumn();
        ImGui::Text("%d", width);
        ImGui::NextColumn();
        ImGui::Text("%d", height);
        ImGui::NextColumn();
        ImGui::Text("%d", mipmapLevels);
        ImGui::NextColumn();
        ImGui::Text("%d KB", sizeKB);
        ImGui::NextColumn();
    }

    if (showViewportWindow)
    {
        Viewport();
    }

    ImGui::Columns(1);
    ImGui::End();

    return true;
}

void TextureEditor::Viewport()
{
    bool open = true;
    if (ImGui::Begin("Texture Detail", &open))
    {
        Resource* selectedResource = App->GetResourcesModule()->RequestResource(selectedTextureUID);
        if (selectedResource)
        {
            const ResourceTexture* selectedTexture = dynamic_cast<ResourceTexture*>(selectedResource);
            if (selectedTexture)
            {
                if (selectedTexture->IsCubemap())
                {
                    ImGui::Text("Cubemap Texture Faces:");
                    const ImVec2 imageSize(128, 128);
                    for (int i = 0; i < 6; i++)
                    {
                        ImGui::Text("Face %d", i);
                        ImGui::SameLine();
                        const unsigned int faceTexID = selectedTexture->GetCubemapFaceID(i);
                        ImGui::Image((ImTextureID)(intptr_t)faceTexID, imageSize);
                        if ((i + 1) % 3 == 0) ImGui::NewLine();
                        else ImGui::SameLine();
                    }
                }
                else
                {
                    ImGui::Text("Channel Toggles:");
                    ImGui::Checkbox("Red", &showR);
                    ImGui::SameLine();
                    ImGui::Checkbox("Green", &showG);
                    ImGui::SameLine();
                    ImGui::Checkbox("Blue", &showB);

                    const ImVec4 tint_color(showR ? 1.0f : 0.0f, showG ? 1.0f : 0.0f, showB ? 1.0f : 0.0f, 1.0f);
                    const ImVec2 imageSize(128, 128);

                    ImGui::Text("Filtered Texture:");
                    ImGui::Image(
                        (ImTextureID)(intptr_t)selectedTexture->GetTextureID(), imageSize, ImVec2(0, 0), ImVec2(1, 1),
                        tint_color
                    );
                }
            }
        }
    }
    ImGui::End();

    if (!open)
    {
        showViewportWindow = false;
        selectedTextureUID = 0;
    }
}
