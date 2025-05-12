#include "FontImporter.h"

#include "Application.h"
#include "FileSystem.h"
#include "LibraryModule.h"
#include "MetaFont.h"
#include "ProjectModule.h"
#include "ResourceFont.h"

namespace FontImporter
{
    UID ImportFont(const char* filePath, const std::string& targetFilePath)
    {
        // Copy font to assets folder
        const std::string relativePath = ASSETS_PATH + FileSystem::GetFileNameWithExtension(filePath);
        const std::string copyPath           = targetFilePath + relativePath;
        if (!FileSystem::Exists(copyPath.c_str()))
        {
            FileSystem::Copy(filePath, copyPath.c_str());
        }

        const std::string name      = FileSystem::GetFileNameWithoutExtension(filePath);

        UID fontUID           = GenerateUID();
        fontUID               = App->GetLibraryModule()->AssignFiletypeUID(fontUID, FileType::Font);
        
        UID prefix                 = fontUID / UID_PREFIX_DIVISOR;
     
        const std::string savePath = App->GetProjectModule()->GetLoadedProjectPath() + METADATA_PATH +
                                     std::to_string(prefix) + FILENAME_SEPARATOR + name + META_EXTENSION;
        UID finalFontUID = App->GetLibraryModule()->GetUIDFromMetaFile(savePath);
        if (finalFontUID == INVALID_UID) finalFontUID = fontUID;
        
        const std::string assetPath = ASSETS_PATH + FileSystem::GetFileNameWithExtension(filePath);

        MetaFont meta(finalFontUID, assetPath);
        meta.Save(name, assetPath);

        CopyFont(filePath, targetFilePath, name, finalFontUID);

        return finalFontUID;
    }

    void CopyFont(
        const std::string& filePath, const std::string& targetFilePath, const std::string& name, const UID sourceUID
    )
    {
        const std::string destination = targetFilePath + FONTS_PATH + std::to_string(sourceUID) + FONT_EXTENSION;
        FileSystem::Copy(filePath.c_str(), destination.c_str());

        App->GetLibraryModule()->AddFont(sourceUID, name);
        App->GetLibraryModule()->AddName(name, sourceUID);
        App->GetLibraryModule()->AddResource(destination, sourceUID);
    }

    ResourceFont* LoadFont(UID fontUID)
    {
        const std::string filePath = App->GetLibraryModule()->GetResourcePath(fontUID);
        ResourceFont* resourceFont = new ResourceFont(fontUID, FileSystem::GetFileNameWithoutExtension(filePath));
        resourceFont->SetFilepath(filePath);

        return resourceFont;
    }
} // namespace FontImporter