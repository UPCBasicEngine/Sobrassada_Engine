#include "LibraryModule.h"

#include "FileSystem.h"
#include "SceneImporter.h"
#include "document.h"
#include "prettywriter.h"
#include "stringbuffer.h"

LibraryModule::LibraryModule() {}

LibraryModule::~LibraryModule() {}

bool LibraryModule::Init()
{
    SceneImporter::CreateLibraryDirectories();

    return true;
}

bool LibraryModule::SaveScene(const char *path) const
{
    // Create doc JSON
    rapidjson::Document doc;
    doc.SetObject();
    rapidjson::Document::AllocatorType &allocator = doc.GetAllocator();

    rapidjson::Value scene(rapidjson::kObjectType);

    // Scene values
    UID uid            = 0;
    std::string name   = "Test Scene";
    UID rootGameObject = 0;

    // Create structure
    scene.AddMember("UID", uid, allocator);
    scene.AddMember("Name", rapidjson::Value(name.c_str(), allocator), allocator);
    scene.AddMember("RootGameObject", rootGameObject, allocator);

    // Serialize GameObjects
    rapidjson::Value gameObjects(rapidjson::kArrayType);

    // 1 gameobject
    rapidjson::Value A(rapidjson::kObjectType);
    A.AddMember("UID", 1001, allocator);
    A.AddMember("ParentUID", 987654321, allocator);

    // Child UUIDs
    rapidjson::Value childUIDsGO(rapidjson::kArrayType);
    childUIDsGO.PushBack(1, allocator).PushBack(2, allocator).PushBack(3, allocator).PushBack(4, allocator);
    A.AddMember("ChildUIDS", childUIDsGO, allocator);
    A.AddMember("RootComponentUID", 2001, allocator);

    gameObjects.PushBack(A, allocator);

    // Add gameObjects to scene
    scene.AddMember("GameObjects", gameObjects, allocator);

    // Serialize Components
    rapidjson::Value components(rapidjson::kArrayType);

    // 1 component
    rapidjson::Value B(rapidjson::kObjectType);

    B.AddMember("UID", 2001, allocator);
    B.AddMember("ParentUID", 1001, allocator);

    // Child UUIDs
    rapidjson::Value childUIDsComp(rapidjson::kArrayType);
    childUIDsComp.PushBack(1, allocator).PushBack(2, allocator).PushBack(3, allocator).PushBack(4, allocator);
    B.AddMember("ChildUIDS", childUIDsComp, allocator);

    B.AddMember("Type", "MeshRenderer", allocator);

    // Dependent
    B.AddMember("MeshUID", 3001, allocator);
    B.AddMember("MaterialUID", 4001, allocator);

    components.PushBack(B, allocator);

    // Add components to scene
    scene.AddMember("Components", components, allocator);

    doc.AddMember("Scene", scene, allocator);

    // Save file as JSON
    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    std::string fileName      = FileSystem::GetFileNameWithoutExtension(path);

    unsigned int bytesWritten = FileSystem::Save(path, buffer.GetString(), buffer.GetSize(), false);
    if (bytesWritten == 0)
    {
        GLOG("Failed to save scene file: %s", path);
        return false;
    }

    GLOG("%s saved as scene", fileName.c_str());

    return true;
}

bool LibraryModule::LoadScene(const char *path)
{
    rapidjson::Document doc;
    bool loaded = FileSystem::LoadJSON(path, doc);

    if (!loaded)
    {
        GLOG("Failed to load scene file: %s", path);
        return false;
    }

    if (!doc.HasMember("Scene") || !doc["Scene"].IsObject())
    {
        GLOG("Invalid scene format: %s", path);
        return false;
    }

    rapidjson::Value &scene = doc["Scene"];

    // Scene values
    UID uid                 = scene["UID"].GetUint64();
    std::string name        = scene["Name"].GetString();
    UID rootGameObject      = scene["RootGameObject"].GetUint64();

    // Deserialize GameObjects
    if (scene.HasMember("GameObjects") && scene["GameObjects"].IsArray())
    {
        const rapidjson::Value &gameObjects = scene["GameObjects"];
        for (rapidjson::SizeType i = 0; i < gameObjects.Size(); i++)
        {
            const rapidjson::Value &gameObject = gameObjects[i];

            UID uidGO                          = gameObject["UID"].GetUint64();
            UID parentuid                      = gameObject["ParentUID"].GetUint64();

            if (gameObject.HasMember("ChildUIDS") && gameObject["ChildUIDS"].IsArray())
            {
                const rapidjson::Value &childUIDs = gameObject["ChildUIDS"];
                for (rapidjson::SizeType j = 0; j < childUIDs.Size(); j++)
                {
                    UID childUID = childUIDs[j].GetUint64();
                }
            }
            UID rootComponentUUID = gameObject["RootComponentUID"].GetUint64();
        }
    }

    // Deserialize Components
    if (scene.HasMember("Components") && scene["Components"].IsArray())
    {
        const rapidjson::Value &components = scene["Components"];

        for (rapidjson::SizeType i = 0; i < components.Size(); i++)
        {
            const rapidjson::Value &component = components[i];

            UID compUID                       = component["UID"].GetUint64();
            UID parentUID                     = component["ParentUID"].GetUint64();
            std::string type                  = component["Type"].GetString();

            // dependent
        }
    }

    GLOG("Scene loaded successfully: %s", name.c_str());
    return true;
}

void LibraryModule::AddTexture(UID textureUID, const std::string &ddsPath)
{
    textureMap[ddsPath] = textureUID; // Map the texture UID to its DDS path
}

void LibraryModule::AddMesh(UID meshUID, const std::string &sobPath)
{
    meshMap[sobPath] = meshUID; // Map the texture UID to its DDS path
}

void LibraryModule::AddMaterial(UID materialUID, const std::string &matPath)
{
    meshMap[matPath] = materialUID; // Map the texture UID to its DDS path
}

UID LibraryModule::GetTextureUID(const std::string &texturePath) const
{

    auto it = textureMap.find(texturePath);
    if (it != textureMap.end())
    {
        return it->second;
    }

    return 0;
}

UID LibraryModule::GetMeshUID(const std::string &meshPath) const
{

    auto it = meshMap.find(meshPath);
    if (it != meshMap.end())
    {
        return it->second; 
    }

    return 0;
}

UID LibraryModule::GetMaterialUID(const std::string &materialPath) const
{

    auto it = materialMap.find(materialPath);
    if (it != materialMap.end())
    {
        return it->second; 
    }

    return 0;

}

const std::string &LibraryModule::GetResourcePath(UID resourceID)
{
    auto it = resourcePathsMap.find(resourceID);
    if (it != resourcePathsMap.end())
    {
        return it->second;
    }

    return "";
}
