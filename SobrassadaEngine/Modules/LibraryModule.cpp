#include "LibraryModule.h"

#include "Application.h"
#include "Component.h"
#include "ComponentUtils.h"
#include "FileSystem.h"
#include "FileSystem/StateMachineManager.h"
#include "GameObject.h"
#include "NavmeshImporter.h"
#include "ProjectModule.h"
#include "Resource.h"
#include "SceneImporter.h"
#include "SceneModule.h"
#include "TextureImporter.h"

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include <filesystem>
#include <fstream>

LibraryModule::LibraryModule()
{
}

LibraryModule::~LibraryModule()
{
}

bool LibraryModule::Init()
{
    if (App->GetProjectModule()->IsProjectLoaded())
    {
        const std::string& engineDefaultPath = ENGINE_DEFAULT_ASSETS;
        SceneImporter::CreateLibraryDirectories(App->GetProjectModule()->GetLoadedProjectPath());
        SceneImporter::CreateLibraryDirectories(engineDefaultPath);
        LoadLibraryMaps(App->GetProjectModule()->GetLoadedProjectPath());
        LoadLibraryMaps(engineDefaultPath);
    }

    return true;
}

// Save         = Path: library/scenes/   | UID: sceneUID     | FileName: scene name
// SaveAs       = Path: fileName          | UID: generate new | FileName: path filename
// SavePlayMode = Path: sceneUID          | UID: sceneUID     | FileName: sceneUID
bool LibraryModule::SaveScene(const char* path, SaveMode saveMode) const
{
    Scene* loadedScene = App->GetSceneModule()->GetScene();

#ifdef GAME
    if (loadedScene != nullptr)
    {
        return true;
    }
#else
    if (loadedScene != nullptr)
    {
        UID sceneUID = loadedScene->GetSceneUID();
        const std::string& sceneName =
            saveMode == SaveMode::SaveAs ? FileSystem::GetFileNameWithoutExtension(path) : loadedScene->GetSceneName();

        if (sceneName == DEFAULT_SCENE_NAME && saveMode == SaveMode::Save) return false;

        // Create doc JSON
        rapidjson::Document doc;
        doc.SetObject();
        rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

        rapidjson::Value scene(rapidjson::kObjectType);

        if (saveMode == SaveMode::SaveAs)
            loadedScene->Save(scene, allocator, saveMode, GenerateUID(), sceneName.c_str());
        else loadedScene->Save(scene, allocator, saveMode);

        doc.AddMember("Scene", scene, allocator);

        // Save file like JSON
        rapidjson::StringBuffer buffer;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
        doc.Accept(writer);

        std::string sceneFilePath;

        if (saveMode == SaveMode::SavePlayMode)
            sceneFilePath = App->GetProjectModule()->GetLoadedProjectPath() + SCENES_PLAY_PATH +
                            std::to_string(sceneUID) + SCENE_EXTENSION;
        else
            sceneFilePath = App->GetProjectModule()->GetLoadedProjectPath() + SCENES_PATH + sceneName + SCENE_EXTENSION;

        unsigned int bytesWritten = (unsigned int
        )FileSystem::Save(sceneFilePath.c_str(), buffer.GetString(), (unsigned int)buffer.GetSize(), false);
        if (bytesWritten == 0)
        {
            GLOG("Failed to save scene file: %s", sceneName.c_str());
            return false;
        }

        // GLOG("%s saved as scene", sceneName.c_str());
        return true;
    }
#endif

    GLOG("No scene loaded");
    return false;
}

bool LibraryModule::LoadScene(const char* file, bool reload) const
{
    rapidjson::Document doc;

    std::string path;
    if (reload) path = App->GetProjectModule()->GetLoadedProjectPath() + SCENES_PLAY_PATH;
    else path = App->GetProjectModule()->GetLoadedProjectPath() + SCENES_PATH;

    bool loaded = FileSystem::LoadJSON((path + std::string(file)).c_str(), doc);

    if (!loaded)
    {
        GLOG("Failed to load scene file: %s", file);
        return false;
    }

    if (!doc.HasMember("Scene") || !doc["Scene"].IsObject())
    {
        GLOG("Invalid scene format: %s", file);
        return false;
    }

    rapidjson::Value& scene = doc["Scene"];

    App->GetSceneModule()->LoadScene(scene, reload);
    return true;
}

bool LibraryModule::LoadLibraryMaps(const std::string& projectPath)
{
    for (const auto& entry : std::filesystem::recursive_directory_iterator(projectPath + METADATA_PATH))
    {
        if (entry.is_regular_file() && (FileSystem::GetFileExtension(entry.path().string()) == META_EXTENSION))
        {
            std::string filePath = entry.path().string();

            rapidjson::Document doc;
            if (!FileSystem::LoadJSON(filePath.c_str(), doc)) continue;

            UID assetUID          = doc["UID"].GetUint64();
            std::string assetName = doc["name"].GetString();
            std::string assetPath = projectPath + doc["assetPath"].GetString();

            UID prefix            = assetUID / UID_PREFIX_DIVISOR;
            std::string libraryPath;

            rapidjson::Value importOptions;
            if (doc.HasMember("importOptions") && doc["importOptions"].IsObject()) importOptions = doc["importOptions"];

            switch (prefix)
            {
            case 11:
                AddMesh(assetUID, assetName);
                AddName(assetName, assetUID);
                libraryPath = projectPath + MESHES_PATH + std::to_string(assetUID) + MESH_EXTENSION;
                if (FileSystem::Exists(libraryPath.c_str())) AddResource(libraryPath, assetUID);
                else SceneImporter::ImportMeshFromMetadata(assetPath, projectPath, assetName, importOptions, assetUID);
                break;
            case 12:
                AddTexture(assetUID, assetName);
                AddName(assetName, assetUID);
                libraryPath = projectPath + TEXTURES_PATH + std::to_string(assetUID) + TEXTURE_EXTENSION;
                if (FileSystem::Exists(libraryPath.c_str())) AddResource(libraryPath, assetUID);
                else TextureImporter::Import(assetPath.c_str(), projectPath, assetUID);
                break;
            case 13:
                AddMaterial(assetUID, assetName);
                AddName(assetName, assetUID);
                libraryPath = projectPath + MATERIALS_PATH + std::to_string(assetUID) + MATERIAL_EXTENSION;
                if (FileSystem::Exists(libraryPath.c_str())) AddResource(libraryPath, assetUID);
                else SceneImporter::ImportMaterialFromMetadata(assetPath, projectPath, assetName, assetUID);
                break;
            case 14:
                AddModel(assetUID, assetName);
                AddName(assetName, assetUID);
                libraryPath = projectPath + MODELS_LIB_PATH + std::to_string(assetUID) + MODEL_EXTENSION;
                if (FileSystem::Exists(libraryPath.c_str())) AddResource(libraryPath, assetUID);
                else SceneImporter::CopyModel(assetPath, projectPath, assetName, assetUID);
                break;
            case 15:
                AddAnimation(assetUID, assetName);
                AddName(assetName, assetUID);
                libraryPath = projectPath + ANIMATIONS_PATH + std::to_string(assetUID) + ANIMATION_EXTENSION;
                if (FileSystem::Exists(libraryPath.c_str())) AddResource(libraryPath, assetUID);
                else
                    SceneImporter::ImportAnimationFromMetadata(
                        assetPath, projectPath, assetName, assetUID, importOptions
                    );
                break;
            case 16:
                AddPrefab(assetUID, assetName);
                AddName(assetName, assetUID);
                libraryPath = projectPath + PREFABS_LIB_PATH + std::to_string(assetUID) + PREFAB_EXTENSION;
                if (FileSystem::Exists(libraryPath.c_str())) AddResource(libraryPath, assetUID);
                else SceneImporter::CopyPrefab(assetPath, projectPath, assetName, assetUID);
                break;
            case 17:
                AddStateMachine(assetUID, assetName);
                AddName(assetName, assetUID);
                libraryPath = projectPath + STATEMACHINES_LIB_PATH + std::to_string(assetUID) + STATEMACHINE_EXTENSION;
                if (FileSystem::Exists(libraryPath.c_str())) AddResource(libraryPath, assetUID);
                else StateMachineManager::CopyMachine(assetPath, projectPath, assetName, assetUID);
                break;
            case 19:
                AddFont(assetUID, assetName);
                AddName(assetName, assetUID);
                libraryPath = projectPath + FONTS_PATH + std::to_string(assetUID) + FONT_EXTENSION;
                if (FileSystem::Exists(libraryPath.c_str())) AddResource(libraryPath, assetUID);
                else SceneImporter::CopyFont(assetPath, projectPath, assetName, assetUID);
                break;
            case 20:
                AddNavmesh(assetUID, assetName);
                AddName(assetName, assetUID);
                libraryPath = projectPath + NAVMESHES_PATH + assetName + NAVMESH_EXTENSION;

                if (FileSystem::Exists(libraryPath.c_str())) AddResource(libraryPath, assetUID);
                else NavmeshImporter::CopyNavmesh(assetPath, projectPath, libraryPath, assetUID);
                break;
            default:
                GLOG("Unknown UID prefix (%s) for: %s", std::to_string(prefix).c_str(), assetName.c_str());
                continue;
            }
        }
    }

    // GLOG("MODELS MAP SIZE: %d", modelMap.size());

    return true;
}

void LibraryModule::GetImportOptions(UID uid, rapidjson::Value& outImportOptions)
{
    SearchImportOptionsFromUID(uid, outImportOptions);
}

UID LibraryModule::GetUIDFromMetaFile(const std::string& filePath) const
{
    if (FileSystem::GetFileExtension(filePath) == META_EXTENSION)
    {
        rapidjson::Document doc;
        if (FileSystem::LoadJSON(filePath.c_str(), doc)) return doc["UID"].GetUint64();
    }
    return INVALID_UID;
}

void LibraryModule::SearchImportOptionsFromUID(UID uid, rapidjson::Value& outImportOptions)
{
    if (cachedMetadataDocuments.empty())
    {
        CreateCacheForMetadata(ENGINE_DEFAULT_ASSETS);
        CreateCacheForMetadata(App->GetProjectModule()->GetLoadedProjectPath());
    }

    if (const auto it = cachedMetadataDocuments.find(uid); it != cachedMetadataDocuments.end())
    {
        rapidjson::Document doc;
        if (!FileSystem::LoadJSON(it->second.c_str(), doc)) return;
        
        outImportOptions = doc["importOptions"];
    }
}

UID LibraryModule::AssignFiletypeUID(UID originalUID, FileType fileType)
{
    UID prefix = 10; // Default prefix "99" for unknown files
    switch (fileType)
    {
    case FileType::Mesh:
        prefix = 11;
        break;
    case FileType::Texture:
        prefix = 12;
        break;
    case FileType::Material:
        prefix = 13;
        break;
    case FileType::Model:
        prefix = 14;
        break;
    case FileType::Animation:
        prefix = 15;
        break;
    case FileType::Prefab:
        prefix = 16;
        break;
    case FileType::StateMachine:
        prefix = 17;
        break;
    case FileType::Font:
        prefix = 19;
        break;
    case FileType::Navmesh:
        prefix = 20;
        break;
    default:
        GLOG("Category: Unknown File Type (10)");
        break;
    }

    // GLOG("%llu", prefix)
    UID final = (prefix * UID_PREFIX_DIVISOR) + (originalUID % UID_PREFIX_DIVISOR);
    // GLOG("%llu", final);
    return final;
}

void LibraryModule::DeletePrefabFiles(UID prefabUID)
{
    const std::string metaPath = App->GetProjectModule()->GetLoadedProjectPath() + METADATA_PATH +
                                 std::to_string((int)ResourceType::Prefab) + FILENAME_SEPARATOR +
                                 GetResourceName(prefabUID) + META_EXTENSION;

    FileSystem::Delete(metaPath.c_str());

    const std::string assetPath = App->GetProjectModule()->GetLoadedProjectPath() + PREFABS_ASSETS_PATH +
                                  GetResourceName(prefabUID) + PREFAB_EXTENSION;

    FileSystem::Delete(assetPath.c_str());

    const std::string& resourcePath = App->GetProjectModule()->GetLoadedProjectPath() + PREFABS_LIB_PATH +
                                      std::to_string(prefabUID) + PREFAB_EXTENSION;
    FileSystem::Delete(resourcePath.c_str());

    prefabMap.erase(GetResourceName(prefabUID));
    namesMap.erase(prefabUID);
    resourcePathsMap.erase(prefabUID);

    App->GetSceneModule()->GetScene()->OverridePrefabs(prefabUID);
}

void LibraryModule::InvalidateMetadataCache()
{
    cachedMetadataDocuments.clear();
}

void LibraryModule::AddTexture(UID textureUID, const std::string& textureName)
{
    textureMap[textureName] = textureUID;
}

void LibraryModule::AddMesh(UID meshUID, const std::string& meshName)
{
    meshMap[meshName] = meshUID;
}

void LibraryModule::AddMaterial(UID materialUID, const std::string& matName)
{
    materialMap[matName] = materialUID;
}

void LibraryModule::AddModel(UID modelUID, const std::string& modelName)
{
    modelMap[modelName] = modelUID;
}

void LibraryModule::AddAnimation(UID animUID, const std::string& animName)
{
    animMap[animName] = animUID;
}

void LibraryModule::AddPrefab(UID prefabUID, const std::string& prefabName)
{
    prefabMap[prefabName] = prefabUID;
}

void LibraryModule::AddFont(UID fontUID, const std::string& fontName)
{
    fontMap[fontName] = fontUID;
}

void LibraryModule::AddNavmesh(UID navmeshUID, const std::string& navmeshName)
{
    navmeshMap[navmeshName] = navmeshUID;
}

void LibraryModule::AddName(const std::string& resourceName, UID resourceUID)
{
    namesMap[resourceUID] = resourceName;
}

void LibraryModule::AddStateMachine(UID stateMachineUID, const std::string& stMachPath)
{
    stateMachineMap[stMachPath] = stateMachineUID;
}

UID LibraryModule::GetTextureUID(const std::string& texturePath) const
{
    auto it = textureMap.find(texturePath);
    if (it != textureMap.end())
    {
        return it->second;
    }

    return INVALID_UID;
}

UID LibraryModule::GetMeshUID(const std::string& meshPath) const
{
    auto it = meshMap.find(meshPath);
    if (it != meshMap.end())
    {
        return it->second;
    }

    return INVALID_UID;
}

UID LibraryModule::GetMaterialUID(const std::string& materialPath) const
{
    auto it = materialMap.find(materialPath);
    if (it != materialMap.end())
    {
        return it->second;
    }

    return INVALID_UID;
}

UID LibraryModule::GetModelUID(const std::string& modelPath) const
{
    auto it = modelMap.find(modelPath);
    if (it != modelMap.end())
    {
        return it->second;
    }

    return INVALID_UID;
}

UID LibraryModule::GetAnimUID(const std::string& animPath) const
{
    auto it = animMap.find(animPath);
    if (it != animMap.end())
    {
        return it->second;
    }

    return INVALID_UID;
}

UID LibraryModule::GetStateMachineUID(const std::string& stMachPath) const

{
    auto it = stateMachineMap.find(stMachPath);
    if (it != stateMachineMap.end())
    {
        return it->second;
    }

    return INVALID_UID;
}

UID LibraryModule::GetNavmeshUID(const std::string& navmeshPath) const
{
    auto it = navmeshMap.find(navmeshPath);
    if (it != navmeshMap.end())
    {
        return it->second;
    }
    return INVALID_UID;
}

const std::string& LibraryModule::GetResourcePath(UID resourceID) const
{
    auto it = resourcePathsMap.find(resourceID);
    if (it != resourcePathsMap.end())
    {
        // GLOG("requested uid: %llu", resourceID);
        // GLOG("obtained path: %s", it->second.c_str());
        return it->second.GetString();
    }

    return emptyString;
}

void LibraryModule::CreateCacheForMetadata(const std::string& path)
{
    for (const auto& entry : std::filesystem::recursive_directory_iterator(path + METADATA_PATH))
    {
        if (entry.is_regular_file() && (FileSystem::GetFileExtension(entry.path().string()) == META_EXTENSION))
        {
            std::string filePath = entry.path().string();
            rapidjson::Document doc;
            if (!FileSystem::LoadJSON(filePath.c_str(), doc)) continue;

            UID assetUID = doc["UID"].GetUint64();
                
            if (doc.HasMember("importOptions") && doc["importOptions"].IsObject())
            {
                cachedMetadataDocuments[assetUID] = entry.path().string();
            }
        }
    }
}

const std::string& LibraryModule::GetResourceName(UID resourceID) const
{
    auto it = namesMap.find(resourceID);
    if (it != namesMap.end())
    {
        // GLOG("requested uid: %llu", resourceID);
        // GLOG("obtained name: %s", it->second.c_str());
        return it->second.GetString();
    }

    return emptyString;
}

void LibraryModule::AddResource(const std::string& resourcePath, UID resourceUID)
{
    resourcePathsMap[resourceUID] = resourcePath;
}