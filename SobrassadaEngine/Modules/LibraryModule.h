#pragma once

#include "Globals.h"
#include "Module.h"

#include "rapidjson/document.h"
#include <string>
#include <unordered_map>

#include "HashString.h"

enum class SaveMode
{
    Save,
    SaveAs,
    SavePlayMode
};

enum class FileType
{
    Mesh,
    Texture,
    Material,
    Scene,
    Model,
    Animation,
    Prefab,
    StateMachine,
    Font,
    Navmesh
};

class LibraryModule : public Module
{

  public:
    LibraryModule();
    ~LibraryModule() override;

    bool Init() override;

    bool SaveScene(const char* path, SaveMode saveMode) const;
    bool LoadScene(const char* fileName, bool reload = false) const;

    bool LoadLibraryMaps(const std::string& projectPath);
    void GetImportOptions(UID uid, rapidjson::Document& doc, rapidjson::Value& importOptions) const;
    UID GetUIDFromMetaFile(const std::string& path) const;
    void SearchImportOptionsFromUID(
        UID uid, const std::string& path, rapidjson::Document& doc, rapidjson::Value& importOptions
    ) const;
    UID AssignFiletypeUID(UID originalUID, FileType fileType);
    void DeletePrefabFiles(UID prefabUID);

    void AddTexture(UID textureUID, const std::string& ddsPath);
    void AddMesh(UID meshUID, const std::string& matPath);
    void AddMaterial(UID materialUID, const std::string& sobPath);
    void AddPrefab(UID prefabUID, const std::string& prefabPath);
    void AddModel(UID modelUID, const std::string& modelPath);
    void AddAnimation(UID animUID, const std::string& animPath);
    void AddStateMachine(UID stateMachineUID, const std::string& stMachPath);
    void AddFont(UID fontUID, const std::string& fontName);
    void AddNavmesh(UID navmeshUID, const std::string& navmeshName);
    void AddName(const std::string& resourceName, UID resourceUID);
    void AddResource(const std::string& resourcePath, UID resourceUID);

    UID GetTextureUID(const std::string& texturePath) const;
    UID GetMeshUID(const std::string& meshPath) const;
    UID GetMaterialUID(const std::string& materialPath) const;
    UID GetModelUID(const std::string& modelPath) const;
    UID GetAnimUID(const std::string& animPath) const;
    UID GetStateMachineUID(const std::string& stMachPath) const;
    UID GetNavmeshUID(const std::string& navmeshPath) const;

    const std::string& GetResourceName(UID resourceID) const;
    const std::string& GetResourcePath(UID resourceID) const;

    const std::unordered_map<HashString, UID>& GetTextureMap() const { return textureMap; }
    const std::unordered_map<HashString, UID>& GetMaterialMap() const { return materialMap; }
    const std::unordered_map<HashString, UID>& GetMeshMap() const { return meshMap; }
    const std::unordered_map<HashString, UID>& GetModelMap() const { return modelMap; }
    const std::unordered_map<HashString, UID>& GetAnimMap() const { return animMap; }
    const std::unordered_map<HashString, UID>& GetPrefabMap() const { return prefabMap; }
    const std::unordered_map<HashString, UID>& GetStateMachineMap() const { return stateMachineMap; }
    const std::unordered_map<HashString, UID>& GetFontMap() const { return fontMap; }
    const std::unordered_map<HashString, UID>& GetNavmeshMap() const { return navmeshMap; }

  private:
    // maps for user visuals | name -> UID
    std::unordered_map<HashString, UID> textureMap;
    std::unordered_map<HashString, UID> materialMap;
    std::unordered_map<HashString, UID> meshMap;
    std::unordered_map<HashString, UID> prefabMap;
    std::unordered_map<HashString, UID> modelMap;
    std::unordered_map<HashString, UID> stateMachineMap;
    std::unordered_map<HashString, UID> animMap;
    std::unordered_map<HashString, UID> fontMap;
    std::unordered_map<HashString, UID> navmeshMap;

    // inversed map          | UID -> name
    std::unordered_map<UID, HashString> namesMap;

    // filled on load and import
    std::unordered_map<UID, HashString> resourcePathsMap; // UID -> library path

    const std::string emptyString = "";
};
